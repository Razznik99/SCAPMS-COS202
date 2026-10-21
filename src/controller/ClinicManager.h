// =============================================================================
//  SCAPMS - Smart Clinic Appointment & Patient Management System
//  File   : src/controller/ClinicManager.h
//  Layer  : Controller
//  Purpose: Facade that the GUI talks to. Owns the database, the appointment
//           rules, the waiting queue, the undo stack and the reports, and
//           enforces validation/exception handling for every operation.
// =============================================================================
#ifndef SCAPMS_CONTROLLER_CLINICMANAGER_H
#define SCAPMS_CONTROLLER_CLINICMANAGER_H

#include <memory>
#include <queue>
#include <stack>
#include <string>
#include <vector>

#include "../model/ClinicDatabase.h"
#include "AdminCommand.h"
#include "AppointmentController.h"
#include "ReportGenerator.h"
#include "SearchEngine.h"
#include "SortEngine.h"

namespace scapms::controller {

/// One patient waiting for consultation.
struct QueueEntry {
    std::string patientId;
    std::string patientName;
    std::string category;
    std::string ticket;
    std::string joinedAt;
    int priority = 3;
    bool priorityCase = false;
};

/// Aggregated numbers shown on the dashboard.
struct DashboardStats {
    std::size_t totalPatients = 0;
    std::size_t generalPatients = 0;
    std::size_t emergencyPatients = 0;
    std::size_t followUpPatients = 0;
    std::size_t totalAppointments = 0;
    std::size_t pendingAppointments = 0;
    std::size_t completedAppointments = 0;
    std::size_t cancelledAppointments = 0;
    std::size_t todayAppointments = 0;
    std::size_t upcomingAppointments = 0;
    std::size_t queueLength = 0;
    std::size_t priorityQueueLength = 0;
    std::size_t undoDepth = 0;
    std::size_t patientsWithPending = 0;
    double revenueCollected = 0.0;
    double revenuePending = 0.0;
};

class ClinicManager {
public:
    explicit ClinicManager(std::string dataDirectory = "data");
    ~ClinicManager() = default;

    ClinicManager(const ClinicManager&) = delete;
    ClinicManager& operator=(const ClinicManager&) = delete;

    // ---- access ------------------------------------------------------------
    [[nodiscard]] model::ClinicDatabase& database() noexcept { return m_database; }
    [[nodiscard]] const model::ClinicDatabase& database() const noexcept { return m_database; }
    [[nodiscard]] AppointmentController& appointmentController() noexcept {
        return m_appointmentController;
    }
    [[nodiscard]] const std::string& dataDirectory() const noexcept { return m_dataDirectory; }
    void setDataDirectory(std::string directory) { m_dataDirectory = std::move(directory); }

    [[nodiscard]] std::vector<model::PatientRecord*> patients() const {
        return m_database.patientPointers();
    }
    [[nodiscard]] model::PatientRecord* patient(const std::string& patientId);
    [[nodiscard]] bool patientIdExists(const std::string& patientId) const;
    /// Next free identifier ("P-0015") - used to pre-fill the registration form.
    [[nodiscard]] std::string generatePatientId();
    [[nodiscard]] std::vector<model::Appointment> appointmentsForPatient(
        const std::string& patientId) const;
    [[nodiscard]] const std::vector<model::MedicalService>& services() const {
        return m_database.services();
    }

    // ---- patient administration (undoable) ---------------------------------
    void registerPatient(std::unique_ptr<model::PatientRecord> patient);
    void updatePatient(const std::string& patientId,
                       std::unique_ptr<model::PatientRecord> updated);
    void deletePatient(const std::string& patientId);

    // ---- appointment administration ---------------------------------------
    model::Appointment scheduleAppointment(const std::string& patientId,
                                           const std::string& date,
                                           const std::string& time,
                                           const std::string& serviceCode,
                                           const std::string& clinician,
                                           const std::string& notes = {});
    void cancelAppointment(const std::string& appointmentId);
    void completeAppointment(const std::string& appointmentId);
    void removeAppointment(const std::string& appointmentId);
    [[nodiscard]] std::string earliestPendingAppointmentDate(
        const std::string& patientId) const;

    // ---- waiting queue (std::queue) ---------------------------------------
    QueueEntry joinQueue(const std::string& patientId);
    bool callNextPatient(QueueEntry& served);
    bool removeFromQueue(const std::string& patientId);
    [[nodiscard]] bool isInQueue(const std::string& patientId) const;
    [[nodiscard]] std::vector<QueueEntry> queueSnapshot() const;
    [[nodiscard]] std::size_t queueLength() const;
    [[nodiscard]] std::size_t priorityQueueLength() const;

    // ---- search & sort -----------------------------------------------------
    [[nodiscard]] SearchResult search(SearchMode mode, const std::string& query) const;
    [[nodiscard]] SearchResult searchIn(const std::vector<model::PatientRecord*>& records,
                                        SearchMode mode,
                                        const std::string& query) const;
    [[nodiscard]] SortOutcome sortPatients(std::vector<model::PatientRecord*>& records,
                                           SortField field,
                                           SortAlgorithm algorithm) const;

    // ---- undo stack --------------------------------------------------------
    [[nodiscard]] bool canUndo() const noexcept { return !m_undoStack.empty(); }
    [[nodiscard]] std::size_t undoDepth() const noexcept { return m_undoStack.size(); }
    std::string undoLastOperation();
    /// Most recent operation first. Temporarily unwinds the stack (the stack
    /// itself cannot be iterated) and restores it before returning.
    [[nodiscard]] std::vector<std::string> undoHistory();
    void clearUndoHistory();

    // ---- reporting & statistics -------------------------------------------
    [[nodiscard]] ReportResult report(ReportType type, const std::string& date = {}) const {
        return m_reportGenerator.generate(type, date);
    }
    [[nodiscard]] DashboardStats stats() const;
    [[nodiscard]] std::vector<std::string> describeAllPatients() const;

    // ---- persistence -------------------------------------------------------
    void save() const;
    void save(const std::string& directory);
    void load(const std::string& directory);
    void load();
    void newSession();

private:
    void pushCommand(std::unique_ptr<AdminCommand> command);
    void clearQueues();

    model::ClinicDatabase m_database;
    AppointmentController m_appointmentController;
    ReportGenerator m_reportGenerator;

    std::queue<QueueEntry> m_normalQueue;
    std::queue<QueueEntry> m_priorityQueue;
    std::stack<std::unique_ptr<AdminCommand>> m_undoStack;

    std::string m_dataDirectory;
    int m_ticketCounter = 0;
};

} // namespace scapms::controller

#endif // SCAPMS_CONTROLLER_CLINICMANAGER_H
