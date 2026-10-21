// =============================================================================
//  SCAPMS - Smart Clinic Appointment & Patient Management System
//  File   : src/model/ClinicDatabase.h
//  Layer  : Model
//  Purpose: In-memory repository. Owns the patient collection (composition),
//           the appointment list and the medical-service catalogue, and keeps
//           a fixed-size array cache of the most recently accessed patients.
//           It knows nothing about the GUI.
// =============================================================================
#ifndef SCAPMS_MODEL_CLINICDATABASE_H
#define SCAPMS_MODEL_CLINICDATABASE_H

#include <memory>
#include <string>
#include <vector>

#include "Appointment.h"
#include "MedicalService.h"
#include "PatientRecord.h"

namespace scapms::model {

class ClinicDatabase {
public:
    /// Fixed-size cache capacity required by the brief (array, not a container).
    static constexpr int kRecentCapacity = 10;

    ClinicDatabase();

    // ---- patients ----------------------------------------------------------
    void addPatient(std::unique_ptr<PatientRecord> patient);
    bool removePatient(const std::string& patientId);
    bool replacePatient(const std::string& patientId,
                        std::unique_ptr<PatientRecord> replacement);

    [[nodiscard]] PatientRecord* findPatientById(const std::string& patientId);
    [[nodiscard]] const PatientRecord* findPatientById(const std::string& patientId) const;

    [[nodiscard]] const std::vector<std::unique_ptr<PatientRecord>>& patients() const noexcept {
        return m_patients;
    }

    /// Non-owning view used by the search/sort engines and by the GUI.
    [[nodiscard]] std::vector<PatientRecord*> patientPointers() const;

    [[nodiscard]] std::size_t patientCount() const noexcept { return m_patients.size(); }

    // ---- appointments ------------------------------------------------------
    void addAppointment(const Appointment& appointment);
    bool removeAppointment(const std::string& appointmentId);
    bool replaceAppointment(const std::string& appointmentId,
                            const Appointment& replacement);

    [[nodiscard]] Appointment* findAppointmentById(const std::string& appointmentId);
    [[nodiscard]] const Appointment* findAppointmentById(const std::string& appointmentId) const;

    [[nodiscard]] std::vector<Appointment>& appointments() noexcept { return m_appointments; }
    [[nodiscard]] const std::vector<Appointment>& appointments() const noexcept {
        return m_appointments;
    }
    [[nodiscard]] std::vector<Appointment> appointmentsForPatient(
        const std::string& patientId) const;
    [[nodiscard]] std::size_t appointmentCount() const noexcept {
        return m_appointments.size();
    }

    // ---- services ----------------------------------------------------------
    [[nodiscard]] const std::vector<MedicalService>& services() const noexcept {
        return m_services;
    }
    [[nodiscard]] const MedicalService* findService(const std::string& code) const;

    // ---- frequently accessed patient cache (fixed-size array) --------------
    void touchRecent(PatientRecord* patient);
    void removeFromRecent(const std::string& patientId);
    [[nodiscard]] std::vector<PatientRecord*> recentPatients() const;
    [[nodiscard]] int recentCount() const noexcept { return m_recentCount; }

    // ---- lifecycle ---------------------------------------------------------
    [[nodiscard]] bool isEmpty() const noexcept;
    void clear();

    // ---- persistence (CSV files) -------------------------------------------
    void saveTo(const std::string& directory) const;
    void loadFrom(const std::string& directory);

    [[nodiscard]] static std::string patientsPath(const std::string& directory);
    [[nodiscard]] static std::string appointmentsPath(const std::string& directory);
    [[nodiscard]] static std::string servicesPath(const std::string& directory);

private:
    void rebuildAssociations();
    void resetRecent();

    std::vector<std::unique_ptr<PatientRecord>> m_patients;
    std::vector<Appointment> m_appointments;
    std::vector<MedicalService> m_services;

    PatientRecord* m_recent[kRecentCapacity]{};
    int m_recentCount = 0;
};

} // namespace scapms::model

#endif // SCAPMS_MODEL_CLINICDATABASE_H
