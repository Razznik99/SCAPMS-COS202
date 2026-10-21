// =============================================================================
//  SCAPMS - Smart Clinic Appointment & Patient Management System
//  File   : src/controller/AppointmentController.h
//  Layer  : Controller
//  Purpose: All appointment rules live here: eligibility, date/time checks,
//           double-booking prevention, status transitions and fee resolution.
// =============================================================================
#ifndef SCAPMS_CONTROLLER_APPOINTMENTCONTROLLER_H
#define SCAPMS_CONTROLLER_APPOINTMENTCONTROLLER_H

#include <string>
#include <vector>

#include "../model/Appointment.h"
#include "../model/ClinicDatabase.h"

namespace scapms::controller {

class AppointmentController {
public:
    explicit AppointmentController(model::ClinicDatabase& database);

    /// Validates and books an appointment. Throws AppointmentException,
    /// NotFoundException or ValidationException on invalid input.
    model::Appointment schedule(const std::string& patientId,
                                const std::string& date,
                                const std::string& time,
                                const std::string& serviceCode,
                                const std::string& clinician,
                                const std::string& notes = {});

    /// Pending -> Cancelled. Throws when already cancelled/completed.
    void cancel(const std::string& appointmentId);

    /// Pending -> Completed.
    void complete(const std::string& appointmentId);

    /// Permanently deletes the appointment and its association.
    bool remove(const std::string& appointmentId);

    [[nodiscard]] model::Appointment* find(const std::string& appointmentId);
    [[nodiscard]] std::vector<model::Appointment> forPatient(
        const std::string& patientId) const;

    /// True when the clinician is already booked at that date/time.
    [[nodiscard]] bool hasConflict(const std::string& clinician,
                                   const std::string& date,
                                   const std::string& time,
                                   const std::string& ignoreAppointmentId = {}) const;

    [[nodiscard]] int countByStatus(model::AppointmentStatus status) const;
    [[nodiscard]] int countOnDate(const std::string& date) const;

    /// True when the patient type may be booked (implements Appointable).
    [[nodiscard]] static bool isAppointable(const model::PatientRecord& patient);
    [[nodiscard]] static std::string eligibilityMessage(const model::PatientRecord& patient);

private:
    void linkToPatient(const model::Appointment& appointment);

    model::ClinicDatabase& m_database;
};

} // namespace scapms::controller

#endif // SCAPMS_CONTROLLER_APPOINTMENTCONTROLLER_H
