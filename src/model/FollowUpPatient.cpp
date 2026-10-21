// =============================================================================
//  File   : src/model/FollowUpPatient.cpp
// =============================================================================
#include "FollowUpPatient.h"

#include "../utils/DateUtils.h"
#include "../utils/Exceptions.h"
#include "../utils/StringUtils.h"

namespace scapms::model {

using scapms::utils::ValidationException;

FollowUpPatient::FollowUpPatient(std::string id,
                                 std::string name,
                                 int age,
                                 std::string contact,
                                 std::string gender,
                                 std::string registeredOn,
                                 std::string originalVisitDate,
                                 int followUpIntervalDays,
                                 std::string referringDoctor)
    : PatientRecord(std::move(id), std::move(name), age, std::move(contact),
                    std::move(gender),
                    registeredOn.empty() ? utils::todayIso() : std::move(registeredOn)),
      m_originalVisitDate(originalVisitDate.empty() ? utils::todayIso()
                                                    : std::move(originalVisitDate)),
      m_followUpIntervalDays(followUpIntervalDays),
      m_referringDoctor(std::move(referringDoctor)) {}

std::string FollowUpPatient::category() const {
    return "Follow-Up";
}

int FollowUpPatient::priorityLevel() const {
    return 2;
}

std::string FollowUpPatient::priorityLabel() const {
    return "Scheduled";
}

double FollowUpPatient::consultationFee() const {
    // Review fee, reduced when the patient returns within the two-week window.
    return qualifiesForDiscount() ? 1000.0 : 1500.0;
}

std::string FollowUpPatient::clinicalSummary() const {
    return "Follow-up review - original visit " + m_originalVisitDate + " - review due " +
           dueDate() + " (" + std::to_string(m_followUpIntervalDays) +
           " day interval) - referring doctor: " + m_referringDoctor + ".";
}

std::string FollowUpPatient::typeTag() const {
    return "FOLLOWUP";
}

std::unique_ptr<PatientRecord> FollowUpPatient::clone() const {
    return std::make_unique<FollowUpPatient>(*this);
}

void FollowUpPatient::validate() const {
    PatientRecord::validate();
    if (!utils::isValidIsoDate(m_originalVisitDate)) {
        throw ValidationException(
            "Original visit date must be a real date in YYYY-MM-DD format.",
            "originalVisitDate");
    }
    if (m_originalVisitDate > utils::todayIso()) {
        throw ValidationException("Original visit date cannot be in the future.",
                                  "originalVisitDate");
    }
    if (m_followUpIntervalDays < 1 || m_followUpIntervalDays > 365) {
        throw ValidationException(
            "Follow-up interval must be between 1 and 365 days (received " +
                std::to_string(m_followUpIntervalDays) + ").",
            "followUpIntervalDays");
    }
    if (utils::trim(m_referringDoctor).empty()) {
        throw ValidationException("Referring doctor must not be empty.",
                                  "referringDoctor");
    }
}

std::vector<std::string> FollowUpPatient::extraFields() const {
    return {m_originalVisitDate, std::to_string(m_followUpIntervalDays), m_referringDoctor};
}

void FollowUpPatient::loadExtraFields(const std::vector<std::string>& extras) {
    if (extras.empty()) {
        return;
    }
    m_originalVisitDate = utils::trim(extras[0]);
    if (extras.size() > 1) {
        try {
            m_followUpIntervalDays = std::stoi(utils::trim(extras[1]));
        } catch (const std::exception&) {
            throw ValidationException("Follow-up interval must be a number.",
                                      "followUpIntervalDays");
        }
    }
    if (extras.size() > 2) {
        m_referringDoctor = utils::trim(extras[2]);
    }
}

std::string FollowUpPatient::dueDate() const {
    return utils::addDays(m_originalVisitDate, m_followUpIntervalDays);
}

bool FollowUpPatient::isOverdue() const {
    return dueDate() < utils::todayIso();
}

int FollowUpPatient::daysUntilDue() const {
    return utils::daysBetween(utils::todayIso(), dueDate());
}

} // namespace scapms::model
