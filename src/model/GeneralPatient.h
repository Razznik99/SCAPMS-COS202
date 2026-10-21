// =============================================================================
//  SCAPMS - Smart Clinic Appointment & Patient Management System
//  File   : src/model/GeneralPatient.h
//  Layer  : Model
//  Purpose: Walk-in / routine patient. Can be booked into the appointment
//           calendar (implements the Appointable interface).
// =============================================================================
#ifndef SCAPMS_MODEL_GENERALPATIENT_H
#define SCAPMS_MODEL_GENERALPATIENT_H

#include <string>
#include <vector>

#include "Appointable.h"
#include "PatientRecord.h"

namespace scapms::model {

class GeneralPatient final : public PatientRecord, public AppointableImpl {
public:
    GeneralPatient(std::string id,
                   std::string name,
                   int age,
                   std::string contact,
                   std::string gender = "Other",
                   std::string registeredOn = {},
                   std::string insuranceProvider = "Self-Pay",
                   std::string chronicCondition = "None");

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

    // ---- GeneralPatient specific behaviour --------------------------------
    [[nodiscard]] const std::string& insuranceProvider() const noexcept {
        return m_insuranceProvider;
    }
    [[nodiscard]] const std::string& chronicCondition() const noexcept {
        return m_chronicCondition;
    }
    void setInsuranceProvider(std::string provider) {
        m_insuranceProvider = std::move(provider);
    }
    void setChronicCondition(std::string condition) {
        m_chronicCondition = std::move(condition);
    }

    /// Insured patients are billed through their provider rather than self-pay.
    [[nodiscard]] bool isInsured() const { return m_insuranceProvider != "Self-Pay"; }

    /// General consultations are capped at 20 minutes per the clinic policy.
    [[nodiscard]] int slotMinutes() const { return 20; }

protected:
    [[nodiscard]] std::vector<std::string> extraFields() const override;

private:
    std::string m_insuranceProvider;
    std::string m_chronicCondition;
};

} // namespace scapms::model

#endif // SCAPMS_MODEL_GENERALPATIENT_H
