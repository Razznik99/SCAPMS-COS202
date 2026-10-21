// =============================================================================
//  File   : src/utils/DateUtils.h
//  Layer  : Utilities
//  Purpose: ISO-8601 date/time helpers. All dates travel through the system as
//           the fixed-width strings "YYYY-MM-DD" / "HH:MM" so that lexical
//           comparison is equivalent to chronological comparison.
// =============================================================================
#ifndef SCAPMS_UTILS_DATEUTILS_H
#define SCAPMS_UTILS_DATEUTILS_H

#include <string>

namespace scapms::utils {

/// Today's local date as "YYYY-MM-DD".
std::string todayIso();

/// Current local time as "HH:MM".
std::string currentTimeHm();

/// Current local timestamp as "YYYY-MM-DD HH:MM:SS".
std::string nowTimestamp();

/// True when the text is a real calendar date in "YYYY-MM-DD" form.
bool isValidIsoDate(const std::string& date);

/// True when the text is a real 24-hour clock time in "HH:MM" form.
bool isValidIsoTime(const std::string& time);

/// Converts a date to a day number (days since 1970-01-01). Returns 0 when invalid.
long toEpochDays(const std::string& date);

/// Adds (or subtracts) whole days to an ISO date.
std::string addDays(const std::string& date, int days);

/// Signed difference in days: to - from.
int daysBetween(const std::string& from, const std::string& to);

/// True when the date is strictly before today.
bool isPastDate(const std::string& date);

/// True when the date equals today.
bool isToday(const std::string& date);

/// Weekday name for an ISO date ("Monday"...), or empty when invalid.
std::string weekdayName(const std::string& date);

} // namespace scapms::utils

#endif // SCAPMS_UTILS_DATEUTILS_H
