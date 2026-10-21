// =============================================================================
//  File   : src/controller/AdminCommand.cpp
// =============================================================================
#include "AdminCommand.h"

#include "../model/Appointable.h"
#include "../utils/DateUtils.h"

namespace scapms::controller {

namespace {

/// Re-establishes the patient <-> appointment association after an undo.
void linkAppointment(model::ClinicDatabase& database,
                     const model::Appointment& appointment) {
    model::PatientRecord* patient = database.findPatientById(appointment.patientId());
    if (patient == nullptr) {
        return;
    }
    if (auto* appointable = dynamic_cast<model::Appointable*>(patient)) {
        appointable->scheduleAppointment(appointment);
    }
}

} // namespace

AdminCommand::AdminCommand(std::string kind)
    : m_kind(std::move(kind)), m_timestamp(scapms::utils::nowTimestamp()) {}

// =============================================================================
//  DeletePatientCommand
// =============================================================================
DeletePatientCommand::DeletePatientCommand(
    model::ClinicDatabase& database, std::unique_ptr<model::PatientRecord> patient,
    std::vector<model::Appointment> removedAppointments)
    : AdminCommand("DELETE_PATIENT"),
      m_database(database),
      m_patient(std::move(patient)),
      m_removedAppointments(std::move(removedAppointments)) {}

bool DeletePatientCommand::undo() {
    if (!m_patient) {
        return false;
    }
    m_database.addPatient(std::move(m_patient));
    for (const auto& appointment : m_removedAppointments) {
        m_database.addAppointment(appointment);
        linkAppointment(m_database, appointment);
    }
    m_removedAppointments.clear();
    return true;
}

std::string DeletePatientCommand::description() const {
    return "Delete patient " + (m_patient ? m_patient->id() + " (" + m_patient->name() + ")"
                                         : std::string("<restored>"));
}

// =============================================================================
//  ModifyPatientCommand
// =============================================================================
ModifyPatientCommand::ModifyPatientCommand(model::ClinicDatabase& database,
                                           std::unique_ptr<model::PatientRecord> before)
    : AdminCommand("MODIFY_PATIENT"),
      m_database(database),
      m_before(std::move(before)),
      m_patientId(m_before ? m_before->id() : std::string()) {}

bool ModifyPatientCommand::undo() {
    if (!m_before) {
        return false;
    }
    return m_database.replacePatient(m_patientId, m_before->clone());
}

std::string ModifyPatientCommand::description() const {
    return "Modify patient " + m_patientId +
           (m_before ? " (" + m_before->name() + ")" : std::string());
}

// =============================================================================
//  RemoveAppointmentCommand
// =============================================================================
RemoveAppointmentCommand::RemoveAppointmentCommand(model::ClinicDatabase& database,
                                                   model::Appointment appointment)
    : AdminCommand("REMOVE_APPOINTMENT"),
      m_database(database),
      m_appointment(std::move(appointment)) {}

bool RemoveAppointmentCommand::undo() {
    m_database.addAppointment(m_appointment);
    linkAppointment(m_database, m_appointment);
    return true;
}

std::string RemoveAppointmentCommand::description() const {
    return "Remove appointment " + m_appointment.id() + " for patient " +
           m_appointment.patientId() + " on " + m_appointment.date();
}

// =============================================================================
//  ModifyAppointmentCommand
// =============================================================================
ModifyAppointmentCommand::ModifyAppointmentCommand(model::ClinicDatabase& database,
                                                   model::Appointment before,
                                                   std::string operation)
    : AdminCommand("MODIFY_APPOINTMENT"),
      m_database(database),
      m_before(std::move(before)),
      m_operation(std::move(operation)) {}

bool ModifyAppointmentCommand::undo() {
    if (!m_database.replaceAppointment(m_before.id(), m_before)) {
        return false;
    }
    if (m_before.isPending()) {
        linkAppointment(m_database, m_before);
    }
    return true;
}

std::string ModifyAppointmentCommand::description() const {
    return m_operation + " " + m_before.id() + " for patient " + m_before.patientId();
}

} // namespace scapms::controller
