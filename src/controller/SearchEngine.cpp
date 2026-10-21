// =============================================================================
//  File   : src/controller/SearchEngine.cpp
//  Layer  : Controller
//  Purpose: Linear, binary and recursive searching implemented by hand.
//           std::find / std::binary_search / std::lower_bound are not used.
// =============================================================================
#include "SearchEngine.h"

#include <chrono>

#include "../utils/Exceptions.h"
#include "../utils/StringUtils.h"

namespace scapms::controller {

using scapms::utils::containsIgnoreCase;
using scapms::utils::equalsIgnoreCase;
using scapms::utils::toLower;
using scapms::utils::trim;
using scapms::utils::ValidationException;

namespace {

class Timer {
public:
    Timer() : m_started(std::chrono::steady_clock::now()) {}

    [[nodiscard]] long long micros() const {
        return std::chrono::duration_cast<std::chrono::microseconds>(
                   std::chrono::steady_clock::now() - m_started)
            .count();
    }

private:
    std::chrono::steady_clock::time_point m_started;
};

} // namespace

SearchResult SearchEngine::linearSearchById(
    const std::vector<model::PatientRecord*>& records, const std::string& patientId) {
    const std::string key = trim(patientId);
    if (key.empty()) {
        throw ValidationException("Enter a patient ID to search for.", "search");
    }

    Timer timer;
    SearchResult result;
    result.algorithm = "Linear Search (by Patient ID)";
    result.examined = records.size();

    for (const auto* record : records) {
        if (record == nullptr) {
            continue;
        }
        ++result.stats.comparisons;
        if (equalsIgnoreCase(record->id(), key)) {
            result.matches.push_back(const_cast<model::PatientRecord*>(record));
            break; // identifiers are unique
        }
    }

    result.stats.micros = timer.micros();
    result.note = result.matches.empty()
                      ? "No patient matched the ID '" + key + "'."
                      : "Exact match found after " +
                            std::to_string(result.stats.comparisons) + " comparison(s).";
    return result;
}

std::size_t SearchEngine::lowerBoundByName(
    const std::vector<model::PatientRecord*>& records, const std::string& nameKey,
    std::size_t& comparisons) {
    std::size_t low = 0;
    std::size_t high = records.size();
    while (low < high) {
        const std::size_t middle = low + (high - low) / 2;
        ++comparisons;
        const std::string middleKey = toLower(records[middle]->name());
        if (middleKey < nameKey) {
            low = middle + 1;
        } else {
            high = middle;
        }
    }
    return low;
}

bool SearchEngine::isSortedByName(const std::vector<model::PatientRecord*>& records) {
    for (std::size_t i = 1; i < records.size(); ++i) {
        const std::string previous = toLower(records[i - 1]->name());
        const std::string current = toLower(records[i]->name());
        if (current < previous) {
            return false;
        }
    }
    return true;
}

bool SearchEngine::isSortedById(const std::vector<model::PatientRecord*>& records) {
    for (std::size_t i = 1; i < records.size(); ++i) {
        if (records[i]->id() < records[i - 1]->id()) {
            return false;
        }
    }
    return true;
}

SearchResult SearchEngine::binarySearchByName(
    const std::vector<model::PatientRecord*>& records, const std::string& name) {
    const std::string key = toLower(trim(name));
    if (key.empty()) {
        throw ValidationException("Enter a patient name to search for.", "search");
    }
    if (!isSortedByName(records)) {
        throw ValidationException(
            "Binary search needs data sorted by patient name. Sort the records first.",
            "search");
    }

    Timer timer;
    SearchResult result;
    result.algorithm = "Binary Search (by Patient Name)";
    result.usedSortedData = true;
    result.examined = records.size();

    const std::size_t start = lowerBoundByName(records, key, result.stats.comparisons);
    for (std::size_t i = start; i < records.size(); ++i) {
        ++result.stats.comparisons;
        if (toLower(records[i]->name()) != key) {
            break;
        }
        result.matches.push_back(const_cast<model::PatientRecord*>(records[i]));
    }

    result.stats.micros = timer.micros();
    result.note = result.matches.empty()
                      ? "Name '" + trim(name) + "' is not present in the sorted index."
                      : "Found " + std::to_string(result.matches.size()) +
                            " exact name match(es) in " +
                            std::to_string(result.stats.comparisons) + " comparisons.";
    return result;
}

SearchResult SearchEngine::binarySearchById(
    const std::vector<model::PatientRecord*>& records, const std::string& patientId) {
    const std::string key = trim(patientId);
    if (key.empty()) {
        throw ValidationException("Enter a patient ID to search for.", "search");
    }
    if (!isSortedById(records)) {
        throw ValidationException(
            "Binary search by ID needs data sorted by patient ID. Sort the records first.",
            "search");
    }

    Timer timer;
    SearchResult result;
    result.algorithm = "Binary Search (by Patient ID)";
    result.usedSortedData = true;
    result.examined = records.size();

    std::size_t low = 0;
    std::size_t high = records.size();
    while (low < high) {
        const std::size_t middle = low + (high - low) / 2;
        ++result.stats.comparisons;
        const std::string& middleKey = records[middle]->id();
        if (middleKey == key) {
            result.matches.push_back(const_cast<model::PatientRecord*>(records[middle]));
            break;
        }
        if (middleKey < key) {
            low = middle + 1;
        } else {
            high = middle;
        }
    }

    result.stats.micros = timer.micros();
    result.note = result.matches.empty()
                      ? "ID '" + key + "' is not present in the sorted index."
                      : "Exact match found in " +
                            std::to_string(result.stats.comparisons) + " comparisons.";
    return result;
}

void SearchEngine::recursiveNameScan(const std::vector<model::PatientRecord*>& records,
                                     const std::string& fragment, std::size_t index,
                                     SearchResult& result) {
    if (index >= records.size()) {
        return; // base case
    }
    const auto* record = records[index];
    if (record != nullptr) {
        ++result.stats.comparisons;
        if (containsIgnoreCase(record->name(), fragment)) {
            result.matches.push_back(const_cast<model::PatientRecord*>(record));
        }
    }
    recursiveNameScan(records, fragment, index + 1, result); // recursive step
}

SearchResult SearchEngine::recursiveSearchByName(
    const std::vector<model::PatientRecord*>& records, const std::string& nameFragment) {
    const std::string fragment = trim(nameFragment);
    if (fragment.empty()) {
        throw ValidationException("Enter at least one character to search for.", "search");
    }

    Timer timer;
    SearchResult result;
    result.algorithm = "Recursive Search (by Patient Name)";
    result.examined = records.size();
    recursiveNameScan(records, fragment, 0, result);
    result.stats.micros = timer.micros();
    result.note = "Recursive scan visited " + std::to_string(records.size()) +
                  " record(s) and matched " + std::to_string(result.matches.size()) +
                  " name(s) containing '" + fragment + "'.";
    return result;
}

SearchResult SearchEngine::searchByCategory(
    const std::vector<model::PatientRecord*>& records, const std::string& category) {
    const std::string key = trim(category);
    if (key.empty()) {
        throw ValidationException("Select a patient category to filter by.", "search");
    }

    Timer timer;
    SearchResult result;
    result.algorithm = "Linear Category Filter";
    result.examined = records.size();
    for (const auto* record : records) {
        if (record == nullptr) {
            continue;
        }
        ++result.stats.comparisons;
        if (equalsIgnoreCase(record->category(), key) ||
            equalsIgnoreCase(record->typeTag(), key)) {
            result.matches.push_back(const_cast<model::PatientRecord*>(record));
        }
    }
    result.stats.micros = timer.micros();
    result.note = "Category '" + key + "' matched " +
                  std::to_string(result.matches.size()) + " patient(s).";
    return result;
}

SearchResult SearchEngine::autoSearch(
    const std::vector<model::PatientRecord*>& records, const std::string& query) {
    // The state of the data decides the algorithm:
    //  * sorted by name  -> binary search is possible and cheaper (O(log n))
    //  * partially matching id -> linear search (data is not sorted by id)
    const std::string key = trim(query);
    if (key.empty()) {
        throw ValidationException("Enter a search value first.", "search");
    }

    if (isSortedByName(records)) {
        SearchResult result = binarySearchByName(records, key);
        result.algorithm = "Auto -> Binary Search (data is sorted by name)";
        if (!result.matches.empty()) {
            return result;
        }
        // No name matched: the query may be an identifier, so fall back to an
        // O(n) identifier scan (the register is not sorted by ID).
        for (const auto* record : records) {
            if (record == nullptr) {
                continue;
            }
            ++result.stats.comparisons;
            if (equalsIgnoreCase(record->id(), key)) {
                result.matches.push_back(const_cast<model::PatientRecord*>(record));
                break;
            }
        }
        result.note = result.matches.empty()
                          ? "The sorted index held no matching name or ID for '" + key +
                                "'."
                          : "The name index missed, but the ID fallback scan found an "
                            "exact match.";
        return result;
    }

    SearchResult result;
    Timer timer;
    result.algorithm = "Auto -> Linear Search (data is unsorted)";
    result.examined = records.size();
    for (const auto* record : records) {
        if (record == nullptr) {
            continue;
        }
        ++result.stats.comparisons;
        if (equalsIgnoreCase(record->id(), key) ||
            containsIgnoreCase(record->name(), key)) {
            result.matches.push_back(const_cast<model::PatientRecord*>(record));
        }
    }
    result.stats.micros = timer.micros();
    result.note = "Data was not sorted by name, so an O(n) linear scan was used "
                  "(ID or name match).";
    return result;
}

std::string SearchEngine::modeName(SearchMode mode) {
    switch (mode) {
    case SearchMode::LinearById:
        return "Linear Search - Patient ID";
    case SearchMode::BinaryByName:
        return "Binary Search - Patient Name";
    case SearchMode::RecursiveByName:
        return "Recursive Search - Patient Name";
    case SearchMode::CategoryFilter:
        return "Category Filter";
    case SearchMode::Auto:
        return "Automatic (data-aware)";
    }
    return "Search";
}

std::string SearchEngine::strategyExplanation(SearchMode mode, std::size_t itemCount) {
    switch (mode) {
    case SearchMode::LinearById:
        return "Linear search is the correct choice for Patient ID because the "
               "register is kept in registration order, not ID order. It is O(n) "
               "but needs no preprocessing and finds the unique ID exactly.";
    case SearchMode::BinaryByName:
        return "Binary search halves the search space on every step (O(log n)) and "
               "is used once the register has been sorted by name; sorting is what "
               "makes the logarithmic search possible.";
    case SearchMode::RecursiveByName:
        return "The recursive search implements partial (contains) matching by "
               "descending through the register one record per call, with the empty "
               "index as its base case - useful for name fragments.";
    case SearchMode::CategoryFilter:
        return "Category search needs to inspect every record, so a linear pass "
               "that compares the virtual category() of each patient is the right "
               "tool; no ordering can help a categorical match.";
    case SearchMode::Auto:
        return "Automatic mode inspects the data first: with " +
               std::to_string(itemCount) +
               " records it uses binary search when the register is already sorted "
               "by name and falls back to a linear scan otherwise.";
    }
    return {};
}

} // namespace scapms::controller
