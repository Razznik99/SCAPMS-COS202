// =============================================================================
//  File   : src/model/PatientFactory.h
//  Layer  : Model
//  Purpose: Creates the correct concrete PatientRecord subclass from persisted
//           data without the persistence layer knowing about subclasses.
// =============================================================================
#ifndef SCAPMS_MODEL_PATIENTFACTORY_H
#define SCAPMS_MODEL_PATIENTFACTORY_H

#include <memory>
#include <string>
#include <vector>

#include "PatientRecord.h"

namespace scapms::model {

class PatientFactory {
public:
    /// Builds a patient from one CSV row. Throws ValidationException when the
    /// row is malformed or the type tag is unknown.
    static std::unique_ptr<PatientRecord> fromCsvLine(const std::string& csvLine);

    /// Builds a patient from already-split CSV fields.
    static std::unique_ptr<PatientRecord> fromFields(const std::vector<std::string>& fields);

    /// True when the tag ("GENERAL", "EMERGENCY", "FOLLOWUP") is supported.
    static bool isKnownType(const std::string& tag);
};

} // namespace scapms::model

#endif // SCAPMS_MODEL_PATIENTFACTORY_H
