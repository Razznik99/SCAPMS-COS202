// =============================================================================
//  File   : src/utils/StringUtils.cpp
// =============================================================================
#include "StringUtils.h"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <sstream>

namespace scapms::utils {

std::string trim(const std::string& text) {
    const auto begin = text.find_first_not_of(" \t\r\n");
    if (begin == std::string::npos) {
        return {};
    }
    const auto end = text.find_last_not_of(" \t\r\n");
    return text.substr(begin, end - begin + 1);
}

std::string toLower(const std::string& text) {
    std::string out = text;
    std::transform(out.begin(), out.end(), out.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return out;
}

bool equalsIgnoreCase(const std::string& a, const std::string& b) {
    return toLower(trim(a)) == toLower(trim(b));
}

bool startsWithIgnoreCase(const std::string& haystack, const std::string& prefix) {
    if (prefix.size() > haystack.size()) {
        return false;
    }
    return toLower(haystack.substr(0, prefix.size())) == toLower(prefix);
}

bool containsIgnoreCase(const std::string& haystack, const std::string& needle) {
    if (needle.empty()) {
        return true;
    }
    return toLower(haystack).find(toLower(needle)) != std::string::npos;
}

std::vector<std::string> split(const std::string& text, char delimiter) {
    std::vector<std::string> parts;
    std::string current;
    for (const char c : text) {
        if (c == delimiter) {
            parts.push_back(current);
            current.clear();
        } else {
            current.push_back(c);
        }
    }
    parts.push_back(current);
    return parts;
}

std::string join(const std::vector<std::string>& parts, const std::string& delimiter) {
    std::string out;
    for (std::size_t i = 0; i < parts.size(); ++i) {
        if (i != 0) {
            out += delimiter;
        }
        out += parts[i];
    }
    return out;
}

std::string escapeCsv(const std::string& field) {
    const bool needsQuotes = field.find_first_of(",\"\r\n") != std::string::npos;
    if (!needsQuotes) {
        return field;
    }
    std::string out = "\"";
    for (const char c : field) {
        if (c == '"') {
            out += "\"\"";
        } else {
            out += c;
        }
    }
    out += '"';
    return out;
}

std::vector<std::string> parseCsvLine(const std::string& line) {
    std::vector<std::string> fields;
    std::string current;
    bool inQuotes = false;

    for (std::size_t i = 0; i < line.size(); ++i) {
        const char c = line[i];
        if (inQuotes) {
            if (c == '"') {
                if (i + 1 < line.size() && line[i + 1] == '"') {
                    current.push_back('"');
                    ++i;
                } else {
                    inQuotes = false;
                }
            } else {
                current.push_back(c);
            }
        } else if (c == '"') {
            inQuotes = true;
        } else if (c == ',') {
            fields.push_back(current);
            current.clear();
        } else if (c != '\r') {
            current.push_back(c);
        }
    }
    fields.push_back(current);
    return fields;
}

std::string formatMoney(double amount) {
    const bool negative = amount < 0.0;
    const double absValue = negative ? -amount : amount;
    const long long whole = static_cast<long long>(absValue);
    const int cents = static_cast<int>((absValue - static_cast<double>(whole)) * 100.0 + 0.5);

    std::string digits = std::to_string(whole);
    std::string grouped;
    int counter = 0;
    for (auto it = digits.rbegin(); it != digits.rend(); ++it) {
        if (counter != 0 && counter % 3 == 0) {
            grouped.push_back(',');
        }
        grouped.push_back(*it);
        ++counter;
    }
    std::reverse(grouped.begin(), grouped.end());

    char buffer[32];
    std::snprintf(buffer, sizeof(buffer), "%s%s.%02d", negative ? "-" : "",
                  grouped.c_str(), cents);
    return buffer;
}

std::string formatDouble(double value, int decimals) {
    char buffer[64];
    std::snprintf(buffer, sizeof(buffer), "%.*f", decimals, value);
    return buffer;
}

bool isAllDigits(const std::string& text) {
    return !text.empty() &&
           std::all_of(text.begin(), text.end(),
                       [](unsigned char c) { return std::isdigit(c) != 0; });
}

} // namespace scapms::utils
