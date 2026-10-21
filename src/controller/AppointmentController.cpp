// =============================================================================
//  File   : src/controller/AppointmentController.cpp
// =============================================================================
#include "AppointmentController.h"

#include <algorithm>

#include "../model/Appointable.h"
#include "../utils/DateUtils.h"
#include "../utils/Exceptions.h"
#include "../utils/IDGenerator.h"
#include "../utils/StringUtils.h"

namespace scapms::controller {

using scapms::utils::AppointmentException;
using scapms::utils::IDGenerator;
using scapms::utils::NotFoundException;
using scapms::utils::trim;
using scapms::utils::ValidationException;

AppointmentController::AppointmentController(model::ClinicDatabase& database)
    : m_database(database) {}

bool AppointmentController::isAppointable(const model::PatientRecord& patient) {
    return dynamic_cast<const model::Appointable*>(&patient) != nullptr;
}

std::string AppointmentController::eligibilityMessage(const model::PatientRecord& patient) {
    if (isAppointable(patient)) {
        return patient.category() + " patients can be booked into the appointment "
                                    "calendar.";
    }
    return patient.category() +
           " patients are walk-in only and cannot be booked. They must join the "
           "waiting queue instead.";
}

model::Appointment AppointmentController::schedule(const std::string& patientId,
                                                   const std::string& date,
                                                   const std::string& time,
                                                   const std::string& serviceCode,
                                                   const std::string& clinician,
                                                   const std::string& notes) {
    model::PatientRecord* patient = m_database.findPatientById(trim(patientId));
    if (patient == nullptr) {
        throw NotFoundException("Patient", trim(patientId));
    }
    if (!isAppointable(*patient)) {
        throw AppointmentException(eligibilityMessage(*patient));
    }

    const std::string cleanDate = trim(date);
    const std::string cleanTime = trim(time);

    if (!scapms::utils::isValidIsoDate(cleanDate)) {
        throw AppointmentException("Invalid appointment date '" + cleanDate +
                                   "'. Use a real date in YYYY-MM-DD format.");
    }
    if (!scapms::utils::isValidIsoTime(cleanTime)) {
        throw AppointmentException("Invalid appointment time '" + cleanTime +
                                   "'. Use a 24-hour time in HH:MM format.");
    }
    if (scapms::utils::isPastDate(cleanDate)) {
        throw AppointmentException("Appointment date " + cleanDate +
                                   " is in the past. Choose today or a later date.");
    }

    const std::string cleanClinician = trim(clinician);
    if (cleanClinician.empty()) {
        throw ValidationException("Enter the clinician who will see the patient.",
                                  "clinician");
    }

    const model::MedicalService* service = m_database.findService(trim(serviceCode));
    if (service == nullptr) {
        throw NotFoundException("Medical service", trim(serviceCode));
    }

    if (hasConflict(cleanClinician, cleanDate, cleanTime)) {
        throw AppointmentException("Dr/Clinician " + cleanClinician +
                                   " already has an appointment on " + cleanDate +
                                   " at " + cleanTime + ". Choose another slot.");
    }

    // A patient cannot hold two active appointments in the same slot.
    for (const auto& existing : m_database.appointments()) {
        if (existing.patientId() == patient->id() && existing.isPending() &&
            existing.date() == cleanDate && existing.time() == cleanTime) {
            throw AppointmentException("This patient already has a pending "
                                       "appointment in that slot.");
        }
    }

    // Consultation fees depend on the patient type; other services use the
    // catalogue price.
    double fee = service->baseFee();
    if (service->category() == "Consultation") {
        fee = patient->consultationFee();
    }

    model::Appointment appointment(
        IDGenerator::instance().nextId("APT"), patient->id(), cleanDate, cleanTime,
        service->code(), cleanClinician, fee, trim(notes),
        model::AppointmentStatus::Pending);
    appointment.validate();

    m_database.addAppointment(appointment);
    linkToPatient(appointment);
    return appointment;
}

void AppointmentController::cancel(const std::string& appointmentId) {
    model::Appointment* appointment = m_database.findAppointmentById(appointmentId);
    if (appointment == nullptr) {
        throw NotFoundException("Appointment", appointmentId);
    }
    if (appointment->isCancelled()) {
        throw AppointmentException("Appointment " + appointmentId +
                                   " has already been cancelled.");
    }
    if (appointment->isCompleted()) {
        throw AppointmentException("Appointment " + appointmentId +
                                   " is already completed and cannot be cancelled.");
    }
    appointment->setStatus(model::AppointmentStatus::Cancelled);
}

void AppointmentController::complete(const std::string& appointmentId) {
    model::Appointment* appointment = m_database.findAppointmentById(appointmentId);
    if (appointment == nullptr) {
        throw NotFoundException("Appointment", appointmentId);
    }
    if (appointment->isCompleted()) {
        throw AppointmentException("Appointment " + appointmentId +
                                   " is already completed.");
    }
    if (appointment->isCancelled()) {
        throw AppointmentException("A cancelled appointment cannot be completed.");
    }
    appointment->setStatus(model::AppointmentStatus::Completed);
}

bool AppointmentController::remove(const std::string& appointmentId) {
    model::Appointment* appointment = m_database.findAppointmentById(appointmentId);
    if (appointment == nullptr) {
        return false;
    }
    model::PatientRecord* patient = m_database.findPatientById(appointment->patientId());
    if (auto* appointable = dynamic_cast<model::Appointable*>(patient)) {
        appointable->cancelAppointment(appointmentId);
    }
    return m_database.removeAppointment(appointmentId);
}

model::Appointment* AppointmentController::find(const std::string& appointmentId) {
    return m_database.findAppointmentById(appointmentId);
}

std::vector<model::Appointment> AppointmentController::forPatient(
    const std::string& patientId) const {
    return m_database.appointmentsForPatient(patientId);
}

bool AppointmentController::hasConflict(const std::string& clinician,
                                        const std::string& date,
                                        const std::string& time,
                                        const std::string& ignoreAppointmentId) const {
    const std::string key = scapms::utils::toLower(trim(clinician));
    for (const auto& appointment : m_database.appointments()) {
        if (!appointment.isPending()) {
            continue;
        }
        if (!ignoreAppointmentId.empty() && appointment.id() == ignoreAppointmentId) {
            continue;
        }
        if (appointment.date() == date && appointment.time() == time &&
            scapms::utils::toLower(appointment.clinician()) == key) {
            return true;
        }
    }
    return false;
}

int AppointmentController::countByStatus(model::AppointmentStatus status) const {
    int count = 0;
    for (const auto& appointment : m_database.appointments()) {
        if (appointment.status() == status) {
            ++count;
        }
    }
    return count;
}

int AppointmentController::countOnDate(const std::string& date) const {
    int count = 0;
    for (const auto& appointment : m_database.appointments()) {
        if (appointment.date() == date) {
            ++count;
        }
    }
    return count;
}

void AppointmentController::linkToPatient(const model::Appointment& appointment) {
    model::PatientRecord* patient = m_database.findPatientById(appointment.patientId());
    if (auto* appointable = dynamic_cast<model::Appointable*>(patient)) {
        appointable->scheduleAppointment(appointment);
    }
}

} // namespace scapms::controller
