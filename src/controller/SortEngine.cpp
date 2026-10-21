// =============================================================================
//  File   : src/controller/SortEngine.cpp
//  Layer  : Controller
//  Purpose: Selection sort, insertion sort, recursive merge sort and quick sort
//           are implemented from scratch. std::sort / std::stable_sort are
//           intentionally not used (see brief: student-implemented algorithms).
// =============================================================================
#include "SortEngine.h"

#include <algorithm>
#include <chrono>
#include <sstream>

#include "../utils/StringUtils.h"

namespace scapms::controller {

namespace {

/// Key used for every comparison; cached strings keep the algorithms simple.
struct SortKey {
    std::string nameKey;
    std::string idKey;
    int age = 0;
    std::string appointmentDate;
};

std::string effectiveDate(const std::string& date) {
    // Patients without a pending appointment are pushed to the end.
    return date.empty() ? "9999-12-31" : date;
}

SortKey makeKey(const model::PatientRecord* record, SortField field,
                const SortEngine::DateLookup& lookup) {
    SortKey key;
    key.nameKey = scapms::utils::toLower(record->name());
    key.idKey = record->id();
    key.age = record->age();
    if (field == SortField::AppointmentDate && lookup) {
        key.appointmentDate = effectiveDate(lookup(record->id()));
    }
    return key;
}

/// Returns true when `left` must be placed before `right`.
bool keyLess(const SortKey& left, const SortKey& right, SortField field) {
    switch (field) {
    case SortField::PatientName:
        if (left.nameKey != right.nameKey) {
            return left.nameKey < right.nameKey;
        }
        return left.idKey < right.idKey;
    case SortField::PatientId:
        return left.idKey < right.idKey;
    case SortField::Age:
        if (left.age != right.age) {
            return left.age < right.age;
        }
        return left.nameKey < right.nameKey;
    case SortField::AppointmentDate:
        if (left.appointmentDate != right.appointmentDate) {
            return left.appointmentDate < right.appointmentDate;
        }
        return left.nameKey < right.nameKey;
    }
    return left.nameKey < right.nameKey;
}

// -----------------------------------------------------------------------------
//  Selection sort - O(n^2) comparisons, O(n) swaps.
// -----------------------------------------------------------------------------
void selectionSort(std::vector<model::PatientRecord*>& records, SortField field,
                   const SortEngine::DateLookup& lookup, SortStats& stats) {
    const std::size_t n = records.size();
    for (std::size_t i = 0; i < n; ++i) {
        std::size_t smallest = i;
        SortKey smallestKey = makeKey(records[i], field, lookup);
        for (std::size_t j = i + 1; j < n; ++j) {
            ++stats.comparisons;
            const SortKey candidate = makeKey(records[j], field, lookup);
            if (keyLess(candidate, smallestKey, field)) {
                smallest = j;
                smallestKey = candidate;
            }
        }
        if (smallest != i) {
            std::swap(records[i], records[smallest]);
            stats.moves += 2;
        }
    }
}

// -----------------------------------------------------------------------------
//  Insertion sort - O(n^2) worst case, excellent on nearly sorted data.
// -----------------------------------------------------------------------------
void insertionSort(std::vector<model::PatientRecord*>& records, SortField field,
                   const SortEngine::DateLookup& lookup, SortStats& stats) {
    const std::size_t n = records.size();
    for (std::size_t i = 1; i < n; ++i) {
        model::PatientRecord* current = records[i];
        const SortKey currentKey = makeKey(current, field, lookup);
        std::size_t j = i;
        while (j > 0) {
            ++stats.comparisons;
            const SortKey previousKey = makeKey(records[j - 1], field, lookup);
            if (!keyLess(currentKey, previousKey, field)) {
                break;
            }
            records[j] = records[j - 1];
            ++stats.moves;
            --j;
        }
        records[j] = current;
        ++stats.moves;
    }
}

// -----------------------------------------------------------------------------
//  Merge sort - recursive divide and conquer, stable, O(n log n).
// -----------------------------------------------------------------------------
void mergeHalves(std::vector<model::PatientRecord*>& records, std::size_t left,
                 std::size_t middle, std::size_t right, SortField field,
                 const SortEngine::DateLookup& lookup, SortStats& stats) {
    std::vector<model::PatientRecord*> buffer;
    buffer.reserve(right - left + 1);

    std::size_t i = left;
    std::size_t j = middle + 1;
    while (i <= middle && j <= right) {
        ++stats.comparisons;
        const std::string leftKey = field == SortField::AppointmentDate && lookup
                                        ? effectiveDate(lookup(records[i]->id()))
                                        : std::string();
        const std::string rightKey = field == SortField::AppointmentDate && lookup
                                         ? effectiveDate(lookup(records[j]->id()))
                                         : std::string();
        bool takeLeft = false;
        if (field == SortField::AppointmentDate && lookup) {
            takeLeft = leftKey <= rightKey;
        } else {
            takeLeft = !keyLess(makeKey(records[j], field, lookup),
                                makeKey(records[i], field, lookup), field);
        }
        if (takeLeft) {
            buffer.push_back(records[i++]);
        } else {
            buffer.push_back(records[j++]);
        }
    }
    while (i <= middle) {
        buffer.push_back(records[i++]);
    }
    while (j <= right) {
        buffer.push_back(records[j++]);
    }

    for (std::size_t k = 0; k < buffer.size(); ++k) {
        records[left + k] = buffer[k];
        ++stats.moves;
    }
}

void mergeSortRange(std::vector<model::PatientRecord*>& records, std::size_t left,
                    std::size_t right, SortField field,
                    const SortEngine::DateLookup& lookup, SortStats& stats) {
    if (left >= right) {
        return;
    }
    const std::size_t middle = left + (right - left) / 2;
    mergeSortRange(records, left, middle, field, lookup, stats);
    mergeSortRange(records, middle + 1, right, field, lookup, stats);
    mergeHalves(records, left, middle, right, field, lookup, stats);
}

void mergeSort(std::vector<model::PatientRecord*>& records, SortField field,
               const SortEngine::DateLookup& lookup, SortStats& stats) {
    if (records.size() < 2) {
        return;
    }
    mergeSortRange(records, 0, records.size() - 1, field, lookup, stats);
}

// -----------------------------------------------------------------------------
//  Quick sort - median-of-three pivot, in place, O(n log n) average.
// -----------------------------------------------------------------------------
std::size_t partitionRange(std::vector<model::PatientRecord*>& records, std::size_t low,
                           std::size_t high, SortField field,
                           const SortEngine::DateLookup& lookup, SortStats& stats) {
    const std::size_t middle = low + (high - low) / 2;

    // Order low / middle / high so that the median lands at `high`.
    ++stats.comparisons;
    if (keyLess(makeKey(records[middle], field, lookup),
                makeKey(records[low], field, lookup), field)) {
        std::swap(records[middle], records[low]);
        stats.moves += 2;
    }
    ++stats.comparisons;
    if (keyLess(makeKey(records[high], field, lookup),
                makeKey(records[middle], field, lookup), field)) {
        std::swap(records[high], records[middle]);
        stats.moves += 2;
    }
    ++stats.comparisons;
    if (keyLess(makeKey(records[middle], field, lookup),
                makeKey(records[low], field, lookup), field)) {
        std::swap(records[middle], records[low]);
        stats.moves += 2;
    }

    model::PatientRecord* pivot = records[high];
    const SortKey pivotKey = makeKey(pivot, field, lookup);
    std::size_t boundary = low;
    for (std::size_t j = low; j < high; ++j) {
        ++stats.comparisons;
        if (keyLess(makeKey(records[j], field, lookup), pivotKey, field)) {
            if (boundary != j) {
                std::swap(records[boundary], records[j]);
                stats.moves += 2;
            }
            ++boundary;
        }
    }
    std::swap(records[boundary], records[high]);
    stats.moves += 2;
    return boundary;
}

void quickSortRange(std::vector<model::PatientRecord*>& records, std::size_t low,
                    std::size_t high, SortField field,
                    const SortEngine::DateLookup& lookup, SortStats& stats) {
    if (low >= high) {
        return;
    }
    const std::size_t pivotIndex =
        partitionRange(records, low, high, field, lookup, stats);
    if (pivotIndex > 0) {
        quickSortRange(records, low, pivotIndex - 1, field, lookup, stats);
    }
    quickSortRange(records, pivotIndex + 1, high, field, lookup, stats);
}

void quickSort(std::vector<model::PatientRecord*>& records, SortField field,
               const SortEngine::DateLookup& lookup, SortStats& stats) {
    if (records.size() < 2) {
        return;
    }
    quickSortRange(records, 0, records.size() - 1, field, lookup, stats);
}

} // namespace

SortOutcome SortEngine::sort(std::vector<model::PatientRecord*>& records,
                             SortField field,
                             SortAlgorithm algorithm,
                             const DateLookup& lookup) {
    SortOutcome outcome;
    outcome.algorithm = algorithm;
    outcome.field = field;
    outcome.itemCount = records.size();
    outcome.algorithmName = algorithmName(algorithm);
    outcome.fieldName = fieldName(field);

    const auto started = std::chrono::steady_clock::now();
    switch (algorithm) {
    case SortAlgorithm::Selection:
        selectionSort(records, field, lookup, outcome.stats);
        break;
    case SortAlgorithm::Insertion:
        insertionSort(records, field, lookup, outcome.stats);
        break;
    case SortAlgorithm::Merge:
        mergeSort(records, field, lookup, outcome.stats);
        break;
    case SortAlgorithm::Quick:
        quickSort(records, field, lookup, outcome.stats);
        break;
    }
    const auto finished = std::chrono::steady_clock::now();
    outcome.stats.micros = std::chrono::duration_cast<std::chrono::microseconds>(
                               finished - started)
                               .count();
    return outcome;
}

std::string SortEngine::algorithmName(SortAlgorithm algorithm) {
    switch (algorithm) {
    case SortAlgorithm::Selection:
        return "Selection Sort";
    case SortAlgorithm::Insertion:
        return "Insertion Sort";
    case SortAlgorithm::Merge:
        return "Merge Sort";
    case SortAlgorithm::Quick:
        return "Quick Sort";
    }
    return "Selection Sort";
}

std::string SortEngine::fieldName(SortField field) {
    switch (field) {
    case SortField::PatientName:
        return "Patient Name";
    case SortField::PatientId:
        return "Patient ID";
    case SortField::Age:
        return "Age";
    case SortField::AppointmentDate:
        return "Appointment Date";
    }
    return "Patient Name";
}

std::string SortEngine::rationale(SortField field, SortAlgorithm algorithm,
                                  std::size_t itemCount) {
    std::ostringstream stream;
    stream << algorithmName(algorithm) << " on " << itemCount << " records by "
           << fieldName(field) << " - ";

    switch (algorithm) {
    case SortAlgorithm::Selection:
        stream << "selection sort performs one pass per position and only O(n) "
                  "swaps, which suits a small, unsorted register (n = "
               << itemCount << ") where write cost dominates.";
        break;
    case SortAlgorithm::Insertion:
        stream << "insertion sort is adaptive: it is near-linear on nearly-sorted "
                  "data (the common case after a previous sort) and stable, so "
                  "equally-aged or same-day patients keep their registration order.";
        break;
    case SortAlgorithm::Merge:
        stream << "merge sort guarantees O(n log n) time and is stable, making it "
                  "the safe choice for large registers and for keeping equal names in "
                  "a predictable order.";
        break;
    case SortAlgorithm::Quick:
        stream << "quick sort sorts in place with a median-of-three pivot, giving "
                  "the best cache behaviour and average O(n log n) time while using "
                  "no auxiliary array.";
        break;
    }
    return stream.str();
}

bool SortEngine::less(const model::PatientRecord* left,
                      const model::PatientRecord* right,
                      SortField field,
                      const DateLookup& lookup) {
    return keyLess(makeKey(left, field, lookup), makeKey(right, field, lookup), field);
}

} // namespace scapms::controller
