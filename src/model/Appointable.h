// =============================================================================
//  SCAPMS - Smart Clinic Appointment & Patient Management System
//  File   : src/model/Appointable.h
//  Layer  : Model
//  Purpose: Interface-like abstract class for patient types that can be booked
//           into the appointment calendar. Emergency patients are walk-in only
//           and deliberately do NOT implement this interface.
// =============================================================================
#ifndef SCAPMS_MODEL_APPOINTABLE_H
#define SCAPMS_MODEL_APPOINTABLE_H

#include <string>
#include <vector>

#include "Appointment.h"

namespace scapms::model {

/// Pure interface: no data, only operations.
class Appointable {
public:
    virtual ~Appointable() = default;

    /// Associates a booked appointment with this patient.
    /// Returns false when the appointment is already linked.
    virtual bool scheduleAppointment(const Appointment& appointment) = 0;

    /// Breaks the association. Returns false when the id is unknown.
    virtual bool cancelAppointment(const std::string& appointmentId) = 0;

    /// All appointment identifiers currently linked to this patient.
    [[nodiscard]] virtual std::vector<std::string> appointmentIds() const = 0;

    [[nodiscard]] virtual std::size_t appointmentCount() const = 0;
};

/// Reusable implementation of the association bookkeeping, shared by the
/// concrete patient types that support appointments (composition of behaviour,
/// no duplicated logic).
class AppointableImpl : public Appointable {
public:
    bool scheduleAppointment(const Appointment& appointment) override;
    bool cancelAppointment(const std::string& appointmentId) override;
    [[nodiscard]] std::vector<std::string> appointmentIds() const override;
    [[nodiscard]] std::size_t appointmentCount() const override;

protected:
    std::vector<std::string> m_appointmentIds;
};

} // namespace scapms::model

#endif // SCAPMS_MODEL_APPOINTABLE_H
