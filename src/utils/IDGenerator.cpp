// =============================================================================
//  File   : src/utils/IDGenerator.cpp
// =============================================================================
#include "IDGenerator.h"

#include <cstdio>
#include <iomanip>
#include <sstream>

#include "StringUtils.h"

namespace scapms::utils {

IDGenerator& IDGenerator::instance() {
    static IDGenerator generator;
    return generator;
}

std::string IDGenerator::nextId(const std::string& prefix) {
    ++m_counters[prefix];
    std::ostringstream stream;
    stream << prefix << '-' << std::setw(kDigits) << std::setfill('0')
           << m_counters[prefix];
    return stream.str();
}

void IDGenerator::observeExistingId(const std::string& existingId) {
    const std::string trimmed = trim(existingId);
    const auto dash = trimmed.rfind('-');
    if (dash == std::string::npos || dash + 1 >= trimmed.size()) {
        return;
    }
    const std::string prefix = trimmed.substr(0, dash);
    const std::string digits = trimmed.substr(dash + 1);
    if (!isAllDigits(digits)) {
        return;
    }
    const int value = std::stoi(digits);
    int& counter = m_counters[prefix];
    if (value > counter) {
        counter = value;
    }
}

void IDGenerator::reset() {
    m_counters.clear();
}

} // namespace scapms::utils
