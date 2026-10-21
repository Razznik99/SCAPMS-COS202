// =============================================================================
//  File   : src/controller/ClinicManager.cpp
// =============================================================================
#include "ClinicManager.h"

#include <algorithm>
#include <iomanip>
#include <sstream>

#include "../model/Appointable.h"
#include "../utils/DateUtils.h"
#include "../utils/Exceptions.h"
#include "../utils/IDGenerator.h"
#include "../utils/StringUtils.h"

namespace scapms::controller {

using scapms::utils::ClinicException;
using scapms::utils::DuplicateIdException;
using scapms::utils::IDGenerator;
using scapms::utils::NotFoundException;
using scapms::utils::ValidationException;

ClinicManager::ClinicManager(std::string dataDirectory)
    : m_appointmentController(m_database),
      m_reportGenerator(m_database),
      m_dataDirectory(std::move(dataDirectory)) {}

// =============================================================================
//  Patient access
// =============================================================================
model::PatientRecord* ClinicManager::patient(const std::string& patientId) {
    model::PatientRecord* record = m_database.findPatientById(patientId);
    if (record != nullptr) {
        m_database.touchRecent(record); // keeps the fixed-size cache warm
    }
    return record;
}

bool ClinicManager::patientIdExists(const std::string& patientId) const {
    return m_database.findPatientById(patientId) != nullptr;
}

std::string ClinicManager::generatePatientId() {
    return IDGenerator::instance().nextId("P");
}

std::vector<model::Appointment> ClinicManager::appointmentsForPatient(
    const std::string& patientId) const {
    return m_database.appointmentsForPatient(patientId);
}

// =============================================================================
//  Patient administration
// =============================================================================
void ClinicManager::registerPatient(std::unique_ptr<model::PatientRecord> patient) {
    if (!patient) {
        throw ValidationException("No patient details were supplied.");
    }
    if (patientIdExists(patient->id())) {
        throw DuplicateIdException(patient->id());
    }
    patient->validate();
    m_database.addPatient(std::move(patient));
}

void ClinicManager::updatePatient(const std::string& patientId,
                                  std::unique_ptr<model::PatientRecord> updated) {
    if (!updated) {
        throw ValidationException("No updated patient details were supplied.");
    }
    model::PatientRecord* existing = m_database.findPatientById(patientId);
    if (existing == nullptr) {
        throw NotFoundException("Patient", patientId);
    }
    if (updated->id() != patientId) {
        throw ValidationException("The patient ID cannot be changed when editing a record.",
                                  "patientId");
    }
    updated->validate();

    std::unique_ptr<model::PatientRecord> before = existing->clone();
    m_database.replacePatient(patientId, std::move(updated));
    pushCommand(std::make_unique<ModifyPatientCommand>(m_database, std::move(before)));
}

void ClinicManager::deletePatient(const std::string& patientId) {
    model::PatientRecord* existing = m_database.findPatientById(patientId);
    if (existing == nullptr) {
        throw NotFoundException("Patient", patientId);
    }

    std::unique_ptr<model::PatientRecord> snapshot = existing->clone();
    std::vector<model::Appointment> removed = m_database.appointmentsForPatient(patientId);

    for (const auto& appointment : removed) {
        m_appointmentController.remove(appointment.id());
    }
    removeFromQueue(patientId);
    if (!m_database.removePatient(patientId)) {
        throw ClinicException("The patient record could not be removed.");
    }

    pushCommand(std::make_unique<DeletePatientCommand>(m_database, std::move(snapshot),
                                                       std::move(removed)));
}

// =============================================================================
//  Appointments
// =============================================================================
model::Appointment ClinicManager::scheduleAppointment(const std::string& patientId,
                                                      const std::string& date,
                                                      const std::string& time,
                                                      const std::string& serviceCode,
                                                      const std::string& clinician,
                                                      const std::string& notes) {
    model::Appointment appointment =
        m_appointmentController.schedule(patientId, date, time, serviceCode, clinician,
                                         notes);
    m_database.touchRecent(m_database.findPatientById(patientId));
    return appointment;
}

void ClinicManager::cancelAppointment(const std::string& appointmentId) {
    model::Appointment* appointment = m_database.findAppointmentById(appointmentId);
    if (appointment == nullptr) {
        throw NotFoundException("Appointment", appointmentId);
    }
    const model::Appointment before = *appointment;
    m_appointmentController.cancel(appointmentId);
    pushCommand(std::make_unique<ModifyAppointmentCommand>(m_database, before, "Cancel"));
}

void ClinicManager::completeAppointment(const std::string& appointmentId) {
    model::Appointment* appointment = m_database.findAppointmentById(appointmentId);
    if (appointment == nullptr) {
        throw NotFoundException("Appointment", appointmentId);
    }
    const model::Appointment before = *appointment;
    m_appointmentController.complete(appointmentId);
    pushCommand(std::make_unique<ModifyAppointmentCommand>(m_database, before, "Complete"));
}

void ClinicManager::removeAppointment(const std::string& appointmentId) {
    model::Appointment* appointment = m_database.findAppointmentById(appointmentId);
    if (appointment == nullptr) {
        throw NotFoundException("Appointment", appointmentId);
    }
    const model::Appointment removed = *appointment;
    m_appointmentController.remove(appointmentId);
    pushCommand(std::make_unique<RemoveAppointmentCommand>(m_database, removed));
}

std::string ClinicManager::earliestPendingAppointmentDate(
    const std::string& patientId) const {
    std::string earliest;
    for (const auto& appointment : m_database.appointments()) {
        if (!appointment.isPending() || appointment.patientId() != patientId) {
            continue;
        }
        if (earliest.empty() || appointment.date() < earliest) {
            earliest = appointment.date();
        }
    }
    return earliest;
}

// =============================================================================
//  Waiting queue
// =============================================================================
QueueEntry ClinicManager::joinQueue(const std::string& patientId) {
    model::PatientRecord* record = patient(patientId);
    if (record == nullptr) {
        throw NotFoundException("Patient", patientId);
    }
    if (isInQueue(patientId)) {
        throw ValidationException(record->name() +
                                  " is already waiting in the queue.");
    }

    QueueEntry entry;
    entry.patientId = record->id();
    entry.patientName = record->name();
    entry.category = record->category();
    entry.priority = record->priorityLevel();
    entry.priorityCase = record->priorityLevel() == 1;
    entry.joinedAt = scapms::utils::nowTimestamp();

    std::ostringstream ticket;
    ticket << "Q-" << std::setw(3) << std::setfill('0') << ++m_ticketCounter;
    entry.ticket = ticket.str();

    if (entry.priorityCase) {
        m_priorityQueue.push(entry);
    } else {
        m_normalQueue.push(entry);
    }
    return entry;
}

bool ClinicManager::callNextPatient(QueueEntry& served) {
    // Emergency (triage) patients are served first, then strict FIFO.
    if (!m_priorityQueue.empty()) {
        served = m_priorityQueue.front();
        m_priorityQueue.pop();
        return true;
    }
    if (!m_normalQueue.empty()) {
        served = m_normalQueue.front();
        m_normalQueue.pop();
        return true;
    }
    return false;
}

bool ClinicManager::removeFromQueue(const std::string& patientId) {
    const auto rebuild = [&patientId](std::queue<QueueEntry>& source,
                                      bool& removed) {
        std::queue<QueueEntry> kept;
        while (!source.empty()) {
            QueueEntry entry = source.front();
            source.pop();
            if (entry.patientId == patientId) {
                removed = true;
                continue;
            }
            kept.push(entry);
        }
        source = std::move(kept);
    };

    bool removed = false;
    rebuild(m_normalQueue, removed);
    rebuild(m_priorityQueue, removed);
    return removed;
}

bool ClinicManager::isInQueue(const std::string& patientId) const {
    const auto contains = [&patientId](std::queue<QueueEntry> copy) {
        while (!copy.empty()) {
            if (copy.front().patientId == patientId) {
                return true;
            }
            copy.pop();
        }
        return false;
    };
    return contains(m_normalQueue) || contains(m_priorityQueue);
}

std::vector<QueueEntry> ClinicManager::queueSnapshot() const {
    std::vector<QueueEntry> entries;
    std::queue<QueueEntry> copy = m_priorityQueue;
    while (!copy.empty()) {
        entries.push_back(copy.front());
        copy.pop();
    }
    copy = m_normalQueue;
    while (!copy.empty()) {
        entries.push_back(copy.front());
        copy.pop();
    }
    return entries;
}

std::size_t ClinicManager::queueLength() const {
    return m_normalQueue.size() + m_priorityQueue.size();
}

std::size_t ClinicManager::priorityQueueLength() const {
    return m_priorityQueue.size();
}

// =============================================================================
//  Search & sort
// =============================================================================
SearchResult ClinicManager::search(SearchMode mode, const std::string& query) const {
    return searchIn(m_database.patientPointers(), mode, query);
}

SearchResult ClinicManager::searchIn(
    const std::vector<model::PatientRecord*>& records, SearchMode mode,
    const std::string& query) const {
    switch (mode) {
    case SearchMode::LinearById:
        return SearchEngine::linearSearchById(records, query);
    case SearchMode::BinaryByName:
        return SearchEngine::binarySearchByName(records, query);
    case SearchMode::RecursiveByName:
        return SearchEngine::recursiveSearchByName(records, query);
    case SearchMode::CategoryFilter:
        return SearchEngine::searchByCategory(records, query);
    case SearchMode::Auto:
        return SearchEngine::autoSearch(records, query);
    }
    return {};
}

SortOutcome ClinicManager::sortPatients(std::vector<model::PatientRecord*>& records,
                                        SortField field,
                                        SortAlgorithm algorithm) const {
    const SortEngine::DateLookup lookup =
        [this](const std::string& patientId) {
            return earliestPendingAppointmentDate(patientId);
        };
    return SortEngine::sort(records, field, algorithm, lookup);
}

// =============================================================================
//  Undo stack
// =============================================================================
void ClinicManager::pushCommand(std::unique_ptr<AdminCommand> command) {
    m_undoStack.push(std::move(command));
}

std::string ClinicManager::undoLastOperation() {
    if (m_undoStack.empty()) {
        throw ValidationException(
            "There is no administrative operation to undo.");
    }
    std::unique_ptr<AdminCommand> command = std::move(m_undoStack.top());
    m_undoStack.pop();
    const std::string description = command->description();
    if (!command->undo()) {
        throw ClinicException("The last operation (" + description +
                              ") could not be reversed.");
    }
    return description;
}

std::vector<std::string> ClinicManager::undoHistory() {
    std::vector<std::string> history;
    std::stack<std::unique_ptr<AdminCommand>> holder;
    while (!m_undoStack.empty()) {
        AdminCommand* command = m_undoStack.top().get();
        history.push_back(command->timestamp() + "  " + command->description() +
                          "  [" + command->kind() + "]");
        holder.push(std::move(m_undoStack.top()));
        m_undoStack.pop();
    }
    while (!holder.empty()) {
        m_undoStack.push(std::move(holder.top()));
        holder.pop();
    }
    return history;
}

void ClinicManager::clearUndoHistory() {
    while (!m_undoStack.empty()) {
        m_undoStack.pop();
    }
}

// =============================================================================
//  Statistics
// =============================================================================
DashboardStats ClinicManager::stats() const {
    DashboardStats stats;
    const std::string today = scapms::utils::todayIso();

    for (const auto& record : m_database.patients()) {
        if (!record) {
            continue;
        }
        ++stats.totalPatients;
        const std::string category = record->category();
        if (category == "General") {
            ++stats.generalPatients;
        } else if (category == "Emergency") {
            ++stats.emergencyPatients;
        } else if (category == "Follow-Up") {
            ++stats.followUpPatients;
        }
    }

    std::vector<std::string> pendingPatients;
    for (const auto& appointment : m_database.appointments()) {
        ++stats.totalAppointments;
        switch (appointment.status()) {
        case model::AppointmentStatus::Pending: {
            ++stats.pendingAppointments;
            stats.revenuePending += appointment.fee();
            if (appointment.date() >= today) {
                ++stats.upcomingAppointments;
            }
            bool alreadyCounted = false;
            for (const auto& patientId : pendingPatients) {
                if (patientId == appointment.patientId()) {
                    alreadyCounted = true;
                    break;
                }
            }
            if (!alreadyCounted) {
                pendingPatients.push_back(appointment.patientId());
            }
            break;
        }
        case model::AppointmentStatus::Completed:
            ++stats.completedAppointments;
            stats.revenueCollected += appointment.fee();
            break;
        case model::AppointmentStatus::Cancelled:
            ++stats.cancelledAppointments;
            break;
        }
        if (appointment.date() == today) {
            ++stats.todayAppointments;
        }
    }

    stats.patientsWithPending = pendingPatients.size();
    stats.queueLength = queueLength();
    stats.priorityQueueLength = priorityQueueLength();
    stats.undoDepth = m_undoStack.size();
    return stats;
}

std::vector<std::string> ClinicManager::describeAllPatients() const {
    // Runtime polymorphism: every virtual call below goes through PatientRecord*.
    std::vector<std::string> lines;
    lines.reserve(m_database.patientCount());
    for (const auto* record : m_database.patientPointers()) {
        lines.push_back(record->describe());
    }
    return lines;
}

// =============================================================================
//  Persistence
// =============================================================================
void ClinicManager::save() const {
    m_database.saveTo(m_dataDirectory);
}

void ClinicManager::save(const std::string& directory) {
    m_dataDirectory = directory;
    m_database.saveTo(m_dataDirectory);
}

void ClinicManager::load(const std::string& directory) {
    m_database.loadFrom(directory);
    m_dataDirectory = directory;
    clearUndoHistory();
    clearQueues();
}

void ClinicManager::load() {
    load(m_dataDirectory);
}

void ClinicManager::newSession() {
    m_database.clear();
    clearUndoHistory();
    clearQueues();
    m_ticketCounter = 0;
    IDGenerator::instance().reset();
}

void ClinicManager::clearQueues() {
    std::queue<QueueEntry> emptyNormal;
    std::queue<QueueEntry> emptyPriority;
    m_normalQueue = std::move(emptyNormal);
    m_priorityQueue = std::move(emptyPriority);
}

} // namespace scapms::controller
