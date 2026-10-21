// =============================================================================
//  SCAPMS - Smart Clinic Appointment & Patient Management System
//  File   : src/model/FollowUpPatient.h
//  Layer  : Model
//  Purpose: Patient returning for a scheduled review of an earlier visit.
//           Supports appointments (Appointable) and knows when the review is due.
// =============================================================================
#ifndef SCAPMS_MODEL_FOLLOWUPPATIENT_H
#define SCAPMS_MODEL_FOLLOWUPPATIENT_H

#include <string>
#include <vector>

#include "Appointable.h"
#include "PatientRecord.h"

namespace scapms::model {

class FollowUpPatient final : public PatientRecord, public AppointableImpl {
public:
    FollowUpPatient(std::string id,
                    std::string name,
                    int age,
                    std::string contact,
                    std::string gender = "Other",
                    std::string registeredOn = {},
                    std::string originalVisitDate = {},
                    int followUpIntervalDays = 14,
                    std::string referringDoctor = "Clinic Duty Doctor");

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

    // ---- FollowUpPatient specific behaviour -------------------------------
    [[nodiscard]] const std::string& originalVisitDate() const noexcept {
        return m_originalVisitDate;
    }
    [[nodiscard]] int followUpIntervalDays() const noexcept { return m_followUpIntervalDays; }
    [[nodiscard]] const std::string& referringDoctor() const noexcept {
        return m_referringDoctor;
    }
    void setOriginalVisitDate(std::string date) { m_originalVisitDate = std::move(date); }
    void setFollowUpIntervalDays(int days) noexcept { m_followUpIntervalDays = days; }
    void setReferringDoctor(std::string doctor) { m_referringDoctor = std::move(doctor); }

    /// Date on which the review becomes due.
    [[nodiscard]] std::string dueDate() const;
    [[nodiscard]] bool isOverdue() const;
    [[nodiscard]] int daysUntilDue() const;

    /// Reviews booked within two weeks of the original visit receive a discount.
    [[nodiscard]] bool qualifiesForDiscount() const { return m_followUpIntervalDays <= 14; }

    [[nodiscard]] int slotMinutes() const { return 15; }

protected:
    [[nodiscard]] std::vector<std::string> extraFields() const override;

private:
    std::string m_originalVisitDate;
    int m_followUpIntervalDays;
    std::string m_referringDoctor;
};

} // namespace scapms::model

#endif // SCAPMS_MODEL_FOLLOWUPPATIENT_H
