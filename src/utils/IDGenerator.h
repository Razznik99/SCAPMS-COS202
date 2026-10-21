// =============================================================================
//  File   : src/utils/IDGenerator.h
//  Layer  : Utilities
//  Purpose: Single source of truth for human-readable identifiers such as
//           "P-0007" (patient) and "APT-0012" (appointment). Counters are
//           re-synchronised whenever existing records are loaded from disk.
// =============================================================================
#ifndef SCAPMS_UTILS_IDGENERATOR_H
#define SCAPMS_UTILS_IDGENERATOR_H

#include <map>
#include <string>

namespace scapms::utils {

class IDGenerator {
public:
    static IDGenerator& instance();

    /// Returns the next identifier for a prefix, e.g. nextId("P") -> "P-0001".
    std::string nextId(const std::string& prefix);

    /// Raises the internal counter so that `existingId` can never be reissued.
    void observeExistingId(const std::string& existingId);

    /// Resets every counter (used by "File -> New").
    void reset();

    /// Width of the numeric part of generated identifiers.
    static constexpr int kDigits = 4;

private:
    IDGenerator() = default;
    std::map<std::string, int> m_counters;
};

} // namespace scapms::utils

#endif // SCAPMS_UTILS_IDGENERATOR_H
