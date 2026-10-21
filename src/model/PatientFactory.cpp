// =============================================================================
//  File   : src/model/PatientFactory.cpp
// =============================================================================
#include "PatientFactory.h"

#include <iostream>

#include "../utils/Exceptions.h"
#include "../utils/StringUtils.h"
#include "EmergencyPatient.h"
#include "FollowUpPatient.h"
#include "GeneralPatient.h"

namespace scapms::model {

using scapms::utils::parseCsvLine;
using scapms::utils::toLower;
using scapms::utils::trim;
using scapms::utils::ValidationException;

bool PatientFactory::isKnownType(const std::string& tag) {
    const std::string value = toLower(trim(tag));
    return value == "general" || value == "emergency" || value == "followup" ||
           value == "follow-up";
}

std::unique_ptr<PatientRecord> PatientFactory::fromCsvLine(const std::string& csvLine) {
    return fromFields(parseCsvLine(csvLine));
}

std::unique_ptr<PatientRecord> PatientFactory::fromFields(
    const std::vector<std::string>& fields) {
    if (fields.size() < 7) {
        throw ValidationException(
            "A patient record needs at least 7 columns (type,id,name,age,gender,"
            "contact,registeredOn) but only " +
            std::to_string(fields.size()) + " were supplied.");
    }

    const std::string type = toLower(trim(fields[0]));
    const std::string id = trim(fields[1]);
    const std::string name = trim(fields[2]);
    const std::string ageText = trim(fields[3]);
    const std::string gender = trim(fields[4]);
    const std::string contact = trim(fields[5]);
    const std::string registeredOn = trim(fields[6]);

    int age = 0;
    try {
        age = std::stoi(ageText);
    } catch (const std::exception&) {
        throw ValidationException("Patient age '" + ageText + "' is not a number.", "age");
    }

    std::vector<std::string> extras(fields.begin() + 7, fields.end());

    std::unique_ptr<PatientRecord> patient;
    if (type == "general") {
        patient = std::make_unique<GeneralPatient>(id, name, age, contact, gender,
                                                   registeredOn);
    } else if (type == "emergency") {
        patient = std::make_unique<EmergencyPatient>(id, name, age, contact, gender,
                                                     registeredOn);
    } else if (type == "followup" || type == "follow-up") {
        patient = std::make_unique<FollowUpPatient>(id, name, age, contact, gender,
                                                    registeredOn);
    } else {
        throw ValidationException("Unknown patient category '" + fields[0] + "'.", "type");
    }

    patient->loadExtraFields(extras);
    patient->validate();
    return patient;
}

} // namespace scapms::model
