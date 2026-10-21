// =============================================================================
//  SCAPMS - Smart Clinic Appointment & Patient Management System
//  File   : src/model/EmergencyPatient.h
//  Layer  : Model
//  Purpose: Emergency / triage patient. Highest clinical priority, fee depends
//           on the triage level, and - by design - does NOT implement
//           Appointable because emergency cases are seen immediately rather
//           than booked into the calendar.
// =============================================================================
#ifndef SCAPMS_MODEL_EMERGENCYPATIENT_H
#define SCAPMS_MODEL_EMERGENCYPATIENT_H

#include <string>
#include <vector>

#include "PatientRecord.h"

namespace scapms::model {

class EmergencyPatient final : public PatientRecord {
public:
    EmergencyPatient(std::string id,
                     std::string name,
                     int age,
                     std::string contact,
                     std::string gender = "Other",
                     std::string registeredOn = {},
                     int triageLevel = 3,
                     std::string arrivalMode = "Walk-in",
                     bool admitted = false);

    // ---- PatientRecord contract -------------------------------------------
    [[nodiscard]] std::string category() const override;
    [[nodiscard]] int priorityLevel() const override;
    [[nodiscard]] std::string priorityLabel() const override;
    [[nodiscard]] double consultationFee() const override;
    [[nodiscard]] std::string clinicalSummary() const override;
    [[nodiscard]] std::string typeTag() const override;
    [[nodiscard]] std::unique_ptr<PatientRecord> clone() const override;

    void validate() const override;
    void loadExtraFields(const std::vector<std::string>& extras) override;

    // ---- EmergencyPatient specific behaviour ------------------------------
    [[nodiscard]] int triageLevel() const noexcept { return m_triageLevel; }
    [[nodiscard]] const std::string& arrivalMode() const noexcept { return m_arrivalMode; }
    [[nodiscard]] bool admitted() const noexcept { return m_admitted; }

    void setTriageLevel(int level) noexcept { m_triageLevel = level; }
    void setArrivalMode(std::string mode) { m_arrivalMode = std::move(mode); }
    void setAdmitted(bool admitted) noexcept { m_admitted = admitted; }

    /// Triage caption used by the GUI delegate and by the queue.
    [[nodiscard]] std::string triageLabel() const;
    [[nodiscard]] bool isCritical() const noexcept { return m_triageLevel <= 2; }

protected:
    [[nodiscard]] std::vector<std::string> extraFields() const override;

private:
    int m_triageLevel;
    std::string m_arrivalMode;
    bool m_admitted;
};

} // namespace scapms::model

#endif // SCAPMS_MODEL_EMERGENCYPATIENT_H
