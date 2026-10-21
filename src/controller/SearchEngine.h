// =============================================================================
//  SCAPMS - Smart Clinic Appointment & Patient Management System
//  File   : src/controller/SearchEngine.h
//  Layer  : Controller
//  Purpose: Student-written searching. Linear search (unsorted data), binary
//           search (sorted data) and a recursive name search. The engine also
//           explains which technique it picked and why.
// =============================================================================
#ifndef SCAPMS_CONTROLLER_SEARCHENGINE_H
#define SCAPMS_CONTROLLER_SEARCHENGINE_H

#include <cstddef>
#include <string>
#include <vector>

#include "../model/PatientRecord.h"

namespace scapms::controller {

enum class SearchMode { LinearById, BinaryByName, RecursiveByName, CategoryFilter, Auto };

struct SearchStats {
    std::size_t comparisons = 0;
    long long micros = 0;
};

struct SearchResult {
    std::vector<model::PatientRecord*> matches;
    SearchStats stats;
    std::string algorithm;
    std::string note;
    std::size_t examined = 0;
    bool usedSortedData = false;
};

class SearchEngine {
public:
    /// Linear search by exact patient id. Works on unsorted data - O(n).
    static SearchResult linearSearchById(const std::vector<model::PatientRecord*>& records,
                                         const std::string& patientId);

    /// Binary search for an exact name. REQUIRES data sorted by name
    /// (case-insensitive); throws ValidationException when it is not sorted.
    static SearchResult binarySearchByName(const std::vector<model::PatientRecord*>& records,
                                           const std::string& name);

    /// Binary search by patient id. Requires data sorted by id.
    static SearchResult binarySearchById(const std::vector<model::PatientRecord*>& records,
                                         const std::string& patientId);

    /// Recursive (self-calling) substring search over patient names.
    static SearchResult recursiveSearchByName(
        const std::vector<model::PatientRecord*>& records, const std::string& nameFragment);

    /// Category / type filter ("General", "Emergency", "Follow-Up").
    static SearchResult searchByCategory(const std::vector<model::PatientRecord*>& records,
                                         const std::string& category);

    /// Chooses the appropriate technique from the state of the data:
    /// binary search when the data is already sorted, linear otherwise.
    static SearchResult autoSearch(const std::vector<model::PatientRecord*>& records,
                                   const std::string& query);

    [[nodiscard]] static bool isSortedByName(
        const std::vector<model::PatientRecord*>& records);
    [[nodiscard]] static bool isSortedById(
        const std::vector<model::PatientRecord*>& records);

    [[nodiscard]] static std::string modeName(SearchMode mode);
    [[nodiscard]] static std::string strategyExplanation(SearchMode mode,
                                                         std::size_t itemCount);

private:
    static void recursiveNameScan(const std::vector<model::PatientRecord*>& records,
                                  const std::string& fragment,
                                  std::size_t index,
                                  SearchResult& result);
    static std::size_t lowerBoundByName(
        const std::vector<model::PatientRecord*>& records, const std::string& nameKey,
        std::size_t& comparisons);
};

} // namespace scapms::controller

#endif // SCAPMS_CONTROLLER_SEARCHENGINE_H
