// =============================================================================
//  File   : src/model/EmergencyPatient.cpp
// =============================================================================
#include "EmergencyPatient.h"

#include "../utils/DateUtils.h"
#include "../utils/Exceptions.h"
#include "../utils/StringUtils.h"

namespace scapms::model {

using scapms::utils::ValidationException;

EmergencyPatient::EmergencyPatient(std::string id,
                                   std::string name,
                                   int age,
                                   std::string contact,
                                   std::string gender,
                                   std::string registeredOn,
                                   int triageLevel,
                                   std::string arrivalMode,
                                   bool admitted)
    : PatientRecord(std::move(id), std::move(name), age, std::move(contact),
                    std::move(gender),
                    registeredOn.empty() ? utils::todayIso() : std::move(registeredOn)),
      m_triageLevel(triageLevel),
      m_arrivalMode(std::move(arrivalMode)),
      m_admitted(admitted) {}

std::string EmergencyPatient::category() const {
    return "Emergency";
}

int EmergencyPatient::priorityLevel() const {
    return 1; // highest
}

std::string EmergencyPatient::priorityLabel() const {
    return isCritical() ? "CRITICAL" : "HIGH";
}

double EmergencyPatient::consultationFee() const {
    // Emergency care carries a triage surcharge: level 1 (+5000) ... level 5 (+1000).
    return 7500.0 + static_cast<double>(6 - m_triageLevel) * 1000.0;
}

std::string EmergencyPatient::clinicalSummary() const {
    return "Emergency case - triage level " + std::to_string(m_triageLevel) + " (" +
           triageLabel() + ") - arrival: " + m_arrivalMode +
           " - admitted: " + (m_admitted ? "yes" : "no") +
           " - walk-in only, not bookable.";
}

std::string EmergencyPatient::typeTag() const {
    return "EMERGENCY";
}

std::unique_ptr<PatientRecord> EmergencyPatient::clone() const {
    return std::make_unique<EmergencyPatient>(*this);
}

void EmergencyPatient::validate() const {
    PatientRecord::validate();
    if (m_triageLevel < 1 || m_triageLevel > 5) {
        throw ValidationException("Triage level must be between 1 and 5 (received " +
                                      std::to_string(m_triageLevel) + ").",
                                  "triageLevel");
    }
    if (m_arrivalMode != "Ambulance" && m_arrivalMode != "Walk-in" &&
        m_arrivalMode != "Referral") {
        throw ValidationException(
            "Arrival mode must be Ambulance, Walk-in or Referral.", "arrivalMode");
    }
}

std::string EmergencyPatient::triageLabel() const {
    switch (m_triageLevel) {
    case 1:
        return "Immediate resuscitation";
    case 2:
        return "Emergent";
    case 3:
        return "Urgent";
    case 4:
        return "Semi-urgent";
    case 5:
        return "Non-urgent";
    default:
        return "Unknown";
    }
}

std::vector<std::string> EmergencyPatient::extraFields() const {
    return {std::to_string(m_triageLevel), m_arrivalMode, m_admitted ? "yes" : "no"};
}

void EmergencyPatient::loadExtraFields(const std::vector<std::string>& extras) {
    if (extras.empty()) {
        return;
    }
    try {
        m_triageLevel = std::stoi(utils::trim(extras[0]));
    } catch (const std::exception&) {
        throw ValidationException("Emergency triage level must be a number.", "triageLevel");
    }
    if (extras.size() > 1) {
        m_arrivalMode = utils::trim(extras[1]);
    }
    if (extras.size() > 2) {
        m_admitted = utils::equalsIgnoreCase(extras[2], "yes") ||
                     utils::equalsIgnoreCase(extras[2], "true") ||
                     utils::trim(extras[2]) == "1";
    }
}

} // namespace scapms::model
