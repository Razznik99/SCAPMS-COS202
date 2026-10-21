// =============================================================================
//  SCAPMS - Smart Clinic Appointment & Patient Management System
//  File   : src/controller/SortEngine.h
//  Layer  : Controller
//  Purpose: Four student-written sorting algorithms over PatientRecord* with
//           four selectable sort keys. No standard library sorting is used.
// =============================================================================
#ifndef SCAPMS_CONTROLLER_SORTENGINE_H
#define SCAPMS_CONTROLLER_SORTENGINE_H

#include <cstddef>
#include <functional>
#include <string>
#include <vector>

#include "../model/PatientRecord.h"

namespace scapms::controller {

enum class SortField { PatientName, PatientId, Age, AppointmentDate };
enum class SortAlgorithm { Selection, Insertion, Merge, Quick };

struct SortStats {
    std::size_t comparisons = 0;
    std::size_t moves = 0;
    long long micros = 0;
};

struct SortOutcome {
    SortAlgorithm algorithm = SortAlgorithm::Selection;
    SortField field = SortField::PatientName;
    SortStats stats;
    std::size_t itemCount = 0;
    std::string algorithmName;
    std::string fieldName;
};

class SortEngine {
public:
    /// Resolves the earliest pending appointment date for a patient id.
    using DateLookup = std::function<std::string(const std::string& patientId)>;

    /// Sorts `records` in place. When sorting by appointment date, `lookup`
    /// must provide "YYYY-MM-DD" (empty string = no appointment, sorts last).
    static SortOutcome sort(std::vector<model::PatientRecord*>& records,
                            SortField field,
                            SortAlgorithm algorithm,
                            const DateLookup& lookup = {});

    [[nodiscard]] static std::string algorithmName(SortAlgorithm algorithm);
    [[nodiscard]] static std::string fieldName(SortField field);

    /// Human-readable justification shown next to the combo boxes.
    [[nodiscard]] static std::string rationale(SortField field,
                                               SortAlgorithm algorithm,
                                               std::size_t itemCount);

    /// Comparator used by the unit tests to verify the ordering.
    static bool less(const model::PatientRecord* left,
                     const model::PatientRecord* right,
                     SortField field,
                     const DateLookup& lookup);
};

} // namespace scapms::controller

#endif // SCAPMS_CONTROLLER_SORTENGINE_H
