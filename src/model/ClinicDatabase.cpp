// =============================================================================
//  File   : src/model/ClinicDatabase.cpp
// =============================================================================
#include "ClinicDatabase.h"

#include <algorithm>
#include <cstddef>
#include <utility>

#include "../utils/DateUtils.h"
#include "../utils/Exceptions.h"
#include "../utils/FileHandler.h"
#include "../utils/IDGenerator.h"
#include "../utils/StringUtils.h"
#include "Appointable.h"
#include "PatientFactory.h"

namespace scapms::model {

using scapms::utils::FileException;
using scapms::utils::FileHandler;
using scapms::utils::IDGenerator;
using scapms::utils::trim;

ClinicDatabase::ClinicDatabase() : m_services(MedicalService::defaults()) {}

// =============================================================================
//  Patients
// =============================================================================
void ClinicDatabase::addPatient(std::unique_ptr<PatientRecord> patient) {
    if (!patient) {
        return;
    }
    IDGenerator::instance().observeExistingId(patient->id());
    m_patients.push_back(std::move(patient));
}

bool ClinicDatabase::removePatient(const std::string& patientId) {
    for (std::size_t i = 0; i < m_patients.size(); ++i) {
        if (m_patients[i] && m_patients[i]->id() == patientId) {
            removeFromRecent(patientId);
            m_patients.erase(m_patients.begin() + static_cast<std::ptrdiff_t>(i));
            return true;
        }
    }
    return false;
}

bool ClinicDatabase::replacePatient(const std::string& patientId,
                                    std::unique_ptr<PatientRecord> replacement) {
    if (!replacement) {
        return false;
    }
    for (auto& record : m_patients) {
        if (record && record->id() == patientId) {
            removeFromRecent(patientId);
            record = std::move(replacement);
            return true;
        }
    }
    return false;
}

PatientRecord* ClinicDatabase::findPatientById(const std::string& patientId) {
    for (const auto& record : m_patients) {
        if (record && record->id() == patientId) {
            return record.get();
        }
    }
    return nullptr;
}

const PatientRecord* ClinicDatabase::findPatientById(const std::string& patientId) const {
    for (const auto& record : m_patients) {
        if (record && record->id() == patientId) {
            return record.get();
        }
    }
    return nullptr;
}

std::vector<PatientRecord*> ClinicDatabase::patientPointers() const {
    std::vector<PatientRecord*> pointers;
    pointers.reserve(m_patients.size());
    for (const auto& record : m_patients) {
        if (record) {
            pointers.push_back(record.get());
        }
    }
    return pointers;
}

// =============================================================================
//  Appointments
// =============================================================================
void ClinicDatabase::addAppointment(const Appointment& appointment) {
    IDGenerator::instance().observeExistingId(appointment.id());
    m_appointments.push_back(appointment);
}

bool ClinicDatabase::removeAppointment(const std::string& appointmentId) {
    for (std::size_t i = 0; i < m_appointments.size(); ++i) {
        if (m_appointments[i].id() == appointmentId) {
            m_appointments.erase(m_appointments.begin() +
                                 static_cast<std::ptrdiff_t>(i));
            return true;
        }
    }
    return false;
}

bool ClinicDatabase::replaceAppointment(const std::string& appointmentId,
                                        const Appointment& replacement) {
    for (auto& appointment : m_appointments) {
        if (appointment.id() == appointmentId) {
            appointment = replacement;
            return true;
        }
    }
    return false;
}

Appointment* ClinicDatabase::findAppointmentById(const std::string& appointmentId) {
    for (auto& appointment : m_appointments) {
        if (appointment.id() == appointmentId) {
            return &appointment;
        }
    }
    return nullptr;
}

const Appointment* ClinicDatabase::findAppointmentById(
    const std::string& appointmentId) const {
    for (const auto& appointment : m_appointments) {
        if (appointment.id() == appointmentId) {
            return &appointment;
        }
    }
    return nullptr;
}

std::vector<Appointment> ClinicDatabase::appointmentsForPatient(
    const std::string& patientId) const {
    std::vector<Appointment> result;
    for (const auto& appointment : m_appointments) {
        if (appointment.patientId() == patientId) {
            result.push_back(appointment);
        }
    }
    return result;
}

// =============================================================================
//  Services
// =============================================================================
const MedicalService* ClinicDatabase::findService(const std::string& code) const {
    for (const auto& service : m_services) {
        if (service.code() == code) {
            return &service;
        }
    }
    return nullptr;
}

// =============================================================================
//  Fixed-size "frequently accessed patients" cache
// =============================================================================
void ClinicDatabase::touchRecent(PatientRecord* patient) {
    if (patient == nullptr) {
        return;
    }

    int found = -1;
    for (int i = 0; i < m_recentCount; ++i) {
        if (m_recent[i] == patient ||
            (m_recent[i] != nullptr && m_recent[i]->id() == patient->id())) {
            found = i;
            break;
        }
    }

    const int limit = found >= 0 ? found
                                 : (m_recentCount < kRecentCapacity ? m_recentCount
                                                                    : kRecentCapacity - 1);
    for (int i = limit; i > 0; --i) {
        m_recent[i] = m_recent[i - 1];
    }
    m_recent[0] = patient;

    if (found < 0 && m_recentCount < kRecentCapacity) {
        ++m_recentCount;
    }
}

void ClinicDatabase::removeFromRecent(const std::string& patientId) {
    for (int i = 0; i < m_recentCount; ++i) {
        if (m_recent[i] != nullptr && m_recent[i]->id() == patientId) {
            for (int j = i; j < m_recentCount - 1; ++j) {
                m_recent[j] = m_recent[j + 1];
            }
            m_recent[m_recentCount - 1] = nullptr;
            --m_recentCount;
            return;
        }
    }
}

std::vector<PatientRecord*> ClinicDatabase::recentPatients() const {
    std::vector<PatientRecord*> result;
    result.reserve(static_cast<std::size_t>(m_recentCount));
    for (int i = 0; i < m_recentCount; ++i) {
        if (m_recent[i] != nullptr) {
            result.push_back(m_recent[i]);
        }
    }
    return result;
}

void ClinicDatabase::resetRecent() {
    for (auto& slot : m_recent) {
        slot = nullptr;
    }
    m_recentCount = 0;
}

// =============================================================================
//  Lifecycle
// =============================================================================
bool ClinicDatabase::isEmpty() const noexcept {
    return m_patients.empty() && m_appointments.empty();
}

void ClinicDatabase::clear() {
    resetRecent();
    m_patients.clear();
    m_appointments.clear();
    m_services = MedicalService::defaults();
}

void ClinicDatabase::rebuildAssociations() {
    for (const auto& appointment : m_appointments) {
        PatientRecord* patient = findPatientById(appointment.patientId());
        if (patient == nullptr) {
            continue;
        }
        if (auto* appointable = dynamic_cast<Appointable*>(patient)) {
            appointable->scheduleAppointment(appointment);
        }
    }
}

// =============================================================================
//  Persistence
// =============================================================================
std::string ClinicDatabase::patientsPath(const std::string& directory) {
    return FileHandler::joinPath(directory, "patients.csv");
}

std::string ClinicDatabase::appointmentsPath(const std::string& directory) {
    return FileHandler::joinPath(directory, "appointments.csv");
}

std::string ClinicDatabase::servicesPath(const std::string& directory) {
    return FileHandler::joinPath(directory, "services.csv");
}

void ClinicDatabase::saveTo(const std::string& directory) const {
    FileHandler::ensureDirectory(directory);

    std::vector<std::string> patientLines;
    patientLines.push_back("# SCAPMS patient records (v1)");
    patientLines.push_back("# type,id,name,age,gender,contact,registeredOn,field1,field2,field3");
    for (const auto& patient : m_patients) {
        if (patient) {
            patientLines.push_back(patient->serialize());
        }
    }
    FileHandler::writeLines(patientsPath(directory), patientLines);

    std::vector<std::string> appointmentLines;
    appointmentLines.push_back("# SCAPMS appointments (v1)");
    appointmentLines.push_back("# id,patientId,date,time,serviceCode,clinician,status,fee,notes");
    for (const auto& appointment : m_appointments) {
        appointmentLines.push_back(appointment.serialize());
    }
    FileHandler::writeLines(appointmentsPath(directory), appointmentLines);

    std::vector<std::string> serviceLines;
    serviceLines.push_back("# SCAPMS medical services (v1)");
    serviceLines.push_back("# code,name,category,durationMinutes,baseFee");
    for (const auto& service : m_services) {
        serviceLines.push_back(service.serialize());
    }
    FileHandler::writeLines(servicesPath(directory), serviceLines);
}

void ClinicDatabase::loadFrom(const std::string& directory) {
    const std::string patientFile = patientsPath(directory);
    const std::string appointmentFile = appointmentsPath(directory);
    const std::string serviceFile = servicesPath(directory);

    if (!FileHandler::exists(patientFile)) {
        throw FileException("The patient data file does not exist", patientFile);
    }

    std::vector<std::unique_ptr<PatientRecord>> loadedPatients;
    std::vector<Appointment> loadedAppointments;
    std::vector<MedicalService> loadedServices;

    // ---- patients ---------------------------------------------------------
    const auto patientLines = FileHandler::readLines(patientFile);
    int lineNumber = 0;
    for (const auto& line : patientLines) {
        ++lineNumber;
        const std::string cleaned = trim(line);
        if (cleaned.empty() || cleaned.front() == '#') {
            continue;
        }
        try {
            loadedPatients.push_back(PatientFactory::fromCsvLine(cleaned));
        } catch (const std::exception& error) {
            throw FileException("Invalid patient record on line " +
                                    std::to_string(lineNumber) + ": " + error.what(),
                                patientFile);
        }
    }

    // ---- appointments -----------------------------------------------------
    if (FileHandler::exists(appointmentFile)) {
        const auto appointmentLines = FileHandler::readLines(appointmentFile);
        lineNumber = 0;
        for (const auto& line : appointmentLines) {
            ++lineNumber;
            const std::string cleaned = trim(line);
            if (cleaned.empty() || cleaned.front() == '#') {
                continue;
            }
            try {
                loadedAppointments.push_back(Appointment::deserialize(cleaned));
            } catch (const std::exception& error) {
                throw FileException("Invalid appointment record on line " +
                                        std::to_string(lineNumber) + ": " + error.what(),
                                    appointmentFile);
            }
        }
    }

    // ---- services (optional: defaults are used when the file is absent) ---
    if (FileHandler::exists(serviceFile)) {
        const auto serviceLines = FileHandler::readLines(serviceFile);
        lineNumber = 0;
        for (const auto& line : serviceLines) {
            ++lineNumber;
            const std::string cleaned = trim(line);
            if (cleaned.empty() || cleaned.front() == '#') {
                continue;
            }
            const auto fields = scapms::utils::parseCsvLine(cleaned);
            if (fields.size() < 5) {
                throw FileException("Invalid service record on line " +
                                        std::to_string(lineNumber),
                                    serviceFile);
            }
            try {
                loadedServices.emplace_back(trim(fields[0]), trim(fields[1]),
                                            trim(fields[2]), std::stoi(trim(fields[3])),
                                            std::stod(trim(fields[4])));
            } catch (const std::exception& error) {
                throw FileException("Invalid service record on line " +
                                        std::to_string(lineNumber) + ": " + error.what(),
                                    serviceFile);
            }
        }
    }

    // ---- commit -----------------------------------------------------------
    resetRecent();
    m_patients = std::move(loadedPatients);
    m_appointments = std::move(loadedAppointments);
    m_services = loadedServices.empty() ? MedicalService::defaults() : std::move(loadedServices);

    for (const auto& patient : m_patients) {
        IDGenerator::instance().observeExistingId(patient->id());
    }
    for (const auto& appointment : m_appointments) {
        IDGenerator::instance().observeExistingId(appointment.id());
    }

    rebuildAssociations();
}

} // namespace scapms::model
