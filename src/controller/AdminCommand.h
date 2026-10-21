// =============================================================================
//  SCAPMS - Smart Clinic Appointment & Patient Management System
//  File   : src/controller/AdminCommand.h
//  Layer  : Controller
//  Purpose: Command objects stored on the std::stack that powers
//           "Undo Last Administrative Operation". Each command knows how to
//           reverse exactly one mutation of the ClinicDatabase.
// =============================================================================
#ifndef SCAPMS_CONTROLLER_ADMINCOMMAND_H
#define SCAPMS_CONTROLLER_ADMINCOMMAND_H

#include <memory>
#include <string>
#include <vector>

#include "../model/Appointment.h"
#include "../model/ClinicDatabase.h"
#include "../model/PatientRecord.h"

namespace scapms::controller {

/// Abstract command - the element type of the undo stack.
class AdminCommand {
public:
    virtual ~AdminCommand() = default;

    /// Reverses the operation. Returns false when the state can no longer be
    /// restored (for example the record was removed elsewhere).
    virtual bool undo() = 0;

    /// Short label shown in the Admin panel history list.
    [[nodiscard]] virtual std::string description() const = 0;

    [[nodiscard]] const std::string& kind() const noexcept { return m_kind; }
    [[nodiscard]] const std::string& timestamp() const noexcept { return m_timestamp; }

protected:
    explicit AdminCommand(std::string kind);

private:
    std::string m_kind;
    std::string m_timestamp;
};

/// Restores a deleted patient together with the appointments that cascaded.
class DeletePatientCommand : public AdminCommand {
public:
    DeletePatientCommand(model::ClinicDatabase& database,
                         std::unique_ptr<model::PatientRecord> patient,
                         std::vector<model::Appointment> removedAppointments);

    bool undo() override;
    [[nodiscard]] std::string description() const override;

private:
    model::ClinicDatabase& m_database;
    std::unique_ptr<model::PatientRecord> m_patient;
    std::vector<model::Appointment> m_removedAppointments;
};

/// Restores the previous version of an edited patient record.
class ModifyPatientCommand : public AdminCommand {
public:
    ModifyPatientCommand(model::ClinicDatabase& database,
                         std::unique_ptr<model::PatientRecord> before);

    bool undo() override;
    [[nodiscard]] std::string description() const override;

private:
    model::ClinicDatabase& m_database;
    std::unique_ptr<model::PatientRecord> m_before;
    std::string m_patientId;
};

/// Restores a permanently removed appointment.
class RemoveAppointmentCommand : public AdminCommand {
public:
    RemoveAppointmentCommand(model::ClinicDatabase& database,
                             model::Appointment appointment);

    bool undo() override;
    [[nodiscard]] std::string description() const override;

private:
    model::ClinicDatabase& m_database;
    model::Appointment m_appointment;
};

/// Restores an appointment's previous state (status, date, clinician...).
class ModifyAppointmentCommand : public AdminCommand {
public:
    ModifyAppointmentCommand(model::ClinicDatabase& database,
                             model::Appointment before,
                             std::string operation);

    bool undo() override;
    [[nodiscard]] std::string description() const override;

private:
    model::ClinicDatabase& m_database;
    model::Appointment m_before;
    std::string m_operation;
};

} // namespace scapms::controller

#endif // SCAPMS_CONTROLLER_ADMINCOMMAND_H
