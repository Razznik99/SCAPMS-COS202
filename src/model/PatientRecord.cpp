// =============================================================================
//  File   : src/model/PatientRecord.cpp
// =============================================================================
#include "PatientRecord.h"

#include <cctype>
#include <sstream>

#include "../utils/DateUtils.h"
#include "../utils/Exceptions.h"
#include "../utils/StringUtils.h"

namespace scapms::model {

using scapms::utils::ValidationException;

PatientRecord::PatientRecord(std::string id,
                             std::string name,
                             int age,
                             std::string contact,
                             std::string gender,
                             std::string registeredOn)
    : m_id(std::move(id)),
      m_name(std::move(name)),
      m_contact(std::move(contact)),
      m_gender(std::move(gender)),
      m_registeredOn(std::move(registeredOn)),
      m_age(age) {}

void PatientRecord::validate() const {
    const std::string trimmedId = utils::trim(m_id);
    if (trimmedId.size() < 3 || trimmedId.size() > 16) {
        throw ValidationException(
            "Patient ID must be between 3 and 16 characters (for example P-0001).",
            "patientId");
    }
    for (const char c : trimmedId) {
        if (!std::isalnum(static_cast<unsigned char>(c)) && c != '-' && c != '_') {
            throw ValidationException(
                "Patient ID may only contain letters, digits, hyphens and "
                "underscores.",
                "patientId");
        }
    }

    const std::string trimmedName = utils::trim(m_name);
    if (trimmedName.size() < 2 || trimmedName.size() > 60) {
        throw ValidationException("Patient name must be between 2 and 60 characters.",
                                  "name");
    }
    for (const char c : trimmedName) {
        if (std::isdigit(static_cast<unsigned char>(c))) {
            throw ValidationException(
                "Patient name must not contain digits: '" + trimmedName + "'.", "name");
        }
    }

    if (m_age < 0 || m_age > 120) {
        throw ValidationException("Patient age must be between 0 and 120 (received " +
                                      std::to_string(m_age) + ").",
                                  "age");
    }

    int digitCount = 0;
    for (const char c : m_contact) {
        if (std::isdigit(static_cast<unsigned char>(c))) {
            ++digitCount;
        } else if (c != '+' && c != '-' && c != ' ' && c != '(' && c != ')' &&
                   c != '/') {
            throw ValidationException(
                "Contact information contains an invalid character: '" +
                    std::string(1, c) + "'. Digits, spaces and + - ( ) / are allowed.",
                "contact");
        }
    }
    if (digitCount < 7 || digitCount > 15) {
        throw ValidationException(
            "Contact information must contain between 7 and 15 digits (found " +
                std::to_string(digitCount) + ").",
            "contact");
    }

    if (m_gender != "Male" && m_gender != "Female" && m_gender != "Other") {
        throw ValidationException("Gender must be Male, Female or Other.", "gender");
    }

    if (!utils::isValidIsoDate(m_registeredOn)) {
        throw ValidationException(
            "Registration date must be a real date in YYYY-MM-DD format.", "registeredOn");
    }
}

bool PatientRecord::isValid() const noexcept {
    try {
        validate();
        return true;
    } catch (const std::exception&) {
        return false;
    }
}

std::string PatientRecord::serialize() const {
    std::vector<std::string> fields;
    fields.push_back(utils::escapeCsv(typeTag()));
    fields.push_back(utils::escapeCsv(m_id));
    fields.push_back(utils::escapeCsv(m_name));
    fields.push_back(std::to_string(m_age));
    fields.push_back(utils::escapeCsv(m_gender));
    fields.push_back(utils::escapeCsv(m_contact));
    fields.push_back(utils::escapeCsv(m_registeredOn));

    for (const auto& extra : extraFields()) {
        fields.push_back(utils::escapeCsv(extra));
    }
    return utils::join(fields, ",");
}

std::string PatientRecord::describe() const {
    std::ostringstream stream;
    stream << m_id << " | " << m_name << " | " << m_age << "y | " << m_gender
           << " | " << m_contact << " | " << category() << " | priority "
           << priorityLevel() << " (" << priorityLabel() << ") | fee "
           << utils::formatDouble(consultationFee(), 2);
    return stream.str();
}

} // namespace scapms::model
