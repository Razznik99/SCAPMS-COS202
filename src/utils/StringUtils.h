// =============================================================================
//  File   : src/utils/StringUtils.h
//  Layer  : Utilities
//  Purpose: Small, dependency-free text helpers shared by the model,
//           controller and persistence layers.
// =============================================================================
#ifndef SCAPMS_UTILS_STRINGUTILS_H
#define SCAPMS_UTILS_STRINGUTILS_H

#include <string>
#include <vector>

namespace scapms::utils {

/// Removes leading and trailing whitespace.
std::string trim(const std::string& text);

/// Lower-cases a copy of the string (ASCII aware, adequate for search keys).
std::string toLower(const std::string& text);

/// Case-insensitive equality.
bool equalsIgnoreCase(const std::string& a, const std::string& b);

/// Case-insensitive "does haystack start with prefix".
bool startsWithIgnoreCase(const std::string& haystack, const std::string& prefix);

/// Case-insensitive substring test used by the recursive search.
bool containsIgnoreCase(const std::string& haystack, const std::string& needle);

/// Splits on a single character. Empty trailing fields are preserved.
std::vector<std::string> split(const std::string& text, char delimiter);

/// Joins fields with a delimiter.
std::string join(const std::vector<std::string>& parts, const std::string& delimiter);

/// Escapes a CSV field (quotes, commas and newlines).
std::string escapeCsv(const std::string& field);

/// Splits one CSV record, honouring quoted fields and doubled quotes.
std::vector<std::string> parseCsvLine(const std::string& line);

/// Formats a currency amount, e.g. 2500 -> "2,500.00".
std::string formatMoney(double amount);

/// Formats a double with a fixed number of decimals.
std::string formatDouble(double value, int decimals);

/// True when the string contains only digits (and is non-empty).
bool isAllDigits(const std::string& text);

} // namespace scapms::utils

#endif // SCAPMS_UTILS_STRINGUTILS_H
