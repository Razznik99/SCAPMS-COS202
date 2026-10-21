// =============================================================================
//  File   : src/model/GeneralPatient.cpp
// =============================================================================
#include "GeneralPatient.h"

#include "../utils/DateUtils.h"
#include "../utils/Exceptions.h"
#include "../utils/StringUtils.h"

namespace scapms::model {

using scapms::utils::ValidationException;

GeneralPatient::GeneralPatient(std::string id,
                               std::string name,
                               int age,
                               std::string contact,
                               std::string gender,
                               std::string registeredOn,
                               std::string insuranceProvider,
                               std::string chronicCondition)
    : PatientRecord(std::move(id), std::move(name), age, std::move(contact),
                    std::move(gender),
                    registeredOn.empty() ? utils::todayIso() : std::move(registeredOn)),
      m_insuranceProvider(std::move(insuranceProvider)),
      m_chronicCondition(std::move(chronicCondition)) {}

std::string GeneralPatient::category() const {
    return "General";
}

int GeneralPatient::priorityLevel() const {
    return 3; // routine
}

std::string GeneralPatient::priorityLabel() const {
    return "Normal";
}

double GeneralPatient::consultationFee() const {
    return 2500.0;
}

std::string GeneralPatient::clinicalSummary() const {
    return "Routine outpatient consultation - 20 minute slot - insurance: " +
           m_insuranceProvider + " - chronic condition: " + m_chronicCondition + ".";
}

std::string GeneralPatient::typeTag() const {
    return "GENERAL";
}

std::unique_ptr<PatientRecord> GeneralPatient::clone() const {
    return std::make_unique<GeneralPatient>(*this);
}

void GeneralPatient::validate() const {
    PatientRecord::validate();
    if (utils::trim(m_insuranceProvider).empty()) {
        throw ValidationException("Insurance provider must not be empty (use "
                                  "'Self-Pay' when uninsured).",
                                  "insuranceProvider");
    }
    if (utils::trim(m_chronicCondition).empty()) {
        throw ValidationException("Chronic condition must not be empty (use 'None').",
                                  "chronicCondition");
    }
}

std::vector<std::string> GeneralPatient::extraFields() const {
    return {m_insuranceProvider, m_chronicCondition};
}

void GeneralPatient::loadExtraFields(const std::vector<std::string>& extras) {
    if (extras.empty()) {
        return;
    }
    m_insuranceProvider = utils::trim(extras[0]);
    if (extras.size() > 1) {
        m_chronicCondition = utils::trim(extras[1]);
    }
}

} // namespace scapms::model
