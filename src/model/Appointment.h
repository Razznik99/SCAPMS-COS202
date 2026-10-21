// =============================================================================
//  SCAPMS - Smart Clinic Appointment & Patient Management System
//  File   : src/model/Appointment.h
//  Layer  : Model
//  Purpose: Value object describing one booked appointment. An appointment is
//           an association between a PatientRecord and a MedicalService.
// =============================================================================
#ifndef SCAPMS_MODEL_APPOINTMENT_H
#define SCAPMS_MODEL_APPOINTMENT_H

#include <string>

namespace scapms::model {

/// Lifecycle of an appointment.
enum class AppointmentStatus { Pending, Completed, Cancelled };

class Appointment {
public:
    Appointment() = default;

    Appointment(std::string id,
                std::string patientId,
                std::string date,
                std::string time,
                std::string serviceCode,
                std::string clinician,
                double fee,
                std::string notes = {},
                AppointmentStatus status = AppointmentStatus::Pending);

    // ---- identity & association -------------------------------------------
    [[nodiscard]] const std::string& id() const noexcept { return m_id; }
    [[nodiscard]] const std::string& patientId() const noexcept { return m_patientId; }
    [[nodiscard]] const std::string& date() const noexcept { return m_date; }
    [[nodiscard]] const std::string& time() const noexcept { return m_time; }
    [[nodiscard]] const std::string& serviceCode() const noexcept { return m_serviceCode; }
    [[nodiscard]] const std::string& clinician() const noexcept { return m_clinician; }
    [[nodiscard]] const std::string& notes() const noexcept { return m_notes; }
    [[nodiscard]] double fee() const noexcept { return m_fee; }
    [[nodiscard]] AppointmentStatus status() const noexcept { return m_status; }

    void setDate(std::string date) { m_date = std::move(date); }
    void setTime(std::string time) { m_time = std::move(time); }
    void setServiceCode(std::string code) { m_serviceCode = std::move(code); }
    void setClinician(std::string clinician) { m_clinician = std::move(clinician); }
    void setNotes(std::string notes) { m_notes = std::move(notes); }
    void setFee(double fee) noexcept { m_fee = fee; }
    void setStatus(AppointmentStatus status) noexcept { m_status = status; }

    // ---- behaviour ---------------------------------------------------------
    /// Field-level validation; throws AppointmentException when invalid.
    void validate() const;

    /// True when every mandatory field is populated and well-formed.
    [[nodiscard]] bool isValid() const noexcept;

    /// "YYYY-MM-DD HH:MM" - directly comparable as a string.
    [[nodiscard]] std::string startKey() const { return m_date + " " + m_time; }

    [[nodiscard]] bool isPending() const noexcept {
        return m_status == AppointmentStatus::Pending;
    }
    [[nodiscard]] bool isCompleted() const noexcept {
        return m_status == AppointmentStatus::Completed;
    }
    [[nodiscard]] bool isCancelled() const noexcept {
        return m_status == AppointmentStatus::Cancelled;
    }

    [[nodiscard]] std::string statusText() const;
    static std::string statusToText(AppointmentStatus status);
    static AppointmentStatus parseStatus(const std::string& text);

    /// CSV representation, one line, no trailing newline.
    [[nodiscard]] std::string serialize() const;
    static Appointment deserialize(const std::string& csvLine);

private:
    std::string m_id;
    std::string m_patientId;
    std::string m_date;
    std::string m_time;
    std::string m_serviceCode;
    std::string m_clinician;
    std::string m_notes;
    double m_fee = 0.0;
    AppointmentStatus m_status = AppointmentStatus::Pending;
};

} // namespace scapms::model

#endif // SCAPMS_MODEL_APPOINTMENT_H
