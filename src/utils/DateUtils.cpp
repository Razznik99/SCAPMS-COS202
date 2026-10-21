// =============================================================================
//  File   : src/utils/DateUtils.cpp
// =============================================================================
#include "DateUtils.h"

#include <array>
#include <cstdio>
#include <ctime>

namespace scapms::utils {
namespace {

bool isLeapYear(int year) {
    return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
}

int daysInMonth(int year, int month) {
    static const std::array<int, 12> kDays{31, 28, 31, 30, 31, 30,
                                           31, 31, 30, 31, 30, 31};
    if (month < 1 || month > 12) {
        return 0;
    }
    if (month == 2 && isLeapYear(year)) {
        return 29;
    }
    return kDays[static_cast<std::size_t>(month - 1)];
}

/// Days since 1970-01-01 for a proleptic Gregorian date (Howard Hinnant's
/// days_from_civil algorithm - no library date support required).
long daysFromCivil(int year, unsigned month, unsigned day) {
    year -= month <= 2 ? 1 : 0;
    const int era = (year >= 0 ? year : year - 399) / 400;
    const unsigned yoe = static_cast<unsigned>(year - era * 400);
    const unsigned doy = (153u * (month + (month > 2 ? -3 : 9)) + 2u) / 5u + day - 1u;
    const unsigned doe = yoe * 365u + yoe / 4u - yoe / 100u + doy;
    return static_cast<long>(era) * 146097L + static_cast<long>(doe) - 719468L;
}

void civilFromDays(long z, int& year, unsigned& month, unsigned& day) {
    z += 719468;
    const long era = (z >= 0 ? z : z - 146096) / 146097;
    const unsigned doe = static_cast<unsigned>(z - era * 146097);
    const unsigned yoe = (doe - doe / 1460u + doe / 36524u - doe / 146096u) / 365u;
    const int y = static_cast<int>(yoe) + static_cast<int>(era) * 400;
    const unsigned doy = doe - (365u * yoe + yoe / 4u - yoe / 100u);
    const unsigned mp = (5u * doy + 2u) / 153u;
    day = doy - (153u * mp + 2u) / 5u + 1u;
    month = mp + (mp < 10 ? 3 : -9);
    year = y + (month <= 2 ? 1 : 0);
}

std::string two(int value) {
    char buffer[8];
    std::snprintf(buffer, sizeof(buffer), "%02d", value);
    return buffer;
}

} // namespace

std::string todayIso() {
    const std::time_t now = std::time(nullptr);
    std::tm local{};
#if defined(_WIN32)
    localtime_s(&local, &now);
#else
    localtime_r(&now, &local);
#endif
    char buffer[16];
    std::snprintf(buffer, sizeof(buffer), "%04d-%02d-%02d", local.tm_year + 1900,
                  local.tm_mon + 1, local.tm_mday);
    return buffer;
}

std::string currentTimeHm() {
    const std::time_t now = std::time(nullptr);
    std::tm local{};
#if defined(_WIN32)
    localtime_s(&local, &now);
#else
    localtime_r(&now, &local);
#endif
    return two(local.tm_hour) + ":" + two(local.tm_min);
}

std::string nowTimestamp() {
    const std::time_t now = std::time(nullptr);
    std::tm local{};
#if defined(_WIN32)
    localtime_s(&local, &now);
#else
    localtime_r(&now, &local);
#endif
    char buffer[24];
    std::snprintf(buffer, sizeof(buffer), "%04d-%02d-%02d %02d:%02d:%02d",
                  local.tm_year + 1900, local.tm_mon + 1, local.tm_mday,
                  local.tm_hour, local.tm_min, local.tm_sec);
    return buffer;
}

bool isValidIsoDate(const std::string& date) {
    if (date.size() != 10 || date[4] != '-' || date[7] != '-') {
        return false;
    }
    for (std::size_t i : {0u, 1u, 2u, 3u, 5u, 6u, 8u, 9u}) {
        if (date[i] < '0' || date[i] > '9') {
            return false;
        }
    }
    const int year = std::stoi(date.substr(0, 4));
    const int month = std::stoi(date.substr(5, 2));
    const int day = std::stoi(date.substr(8, 2));
    if (year < 1900 || year > 2200 || month < 1 || month > 12 || day < 1) {
        return false;
    }
    return day <= daysInMonth(year, month);
}

bool isValidIsoTime(const std::string& time) {
    if (time.size() != 5 || time[2] != ':') {
        return false;
    }
    for (std::size_t i : {0u, 1u, 3u, 4u}) {
        if (time[i] < '0' || time[i] > '9') {
            return false;
        }
    }
    const int hour = std::stoi(time.substr(0, 2));
    const int minute = std::stoi(time.substr(3, 2));
    return hour >= 0 && hour <= 23 && minute >= 0 && minute <= 59;
}

long toEpochDays(const std::string& date) {
    if (!isValidIsoDate(date)) {
        return 0;
    }
    const int year = std::stoi(date.substr(0, 4));
    const unsigned month = static_cast<unsigned>(std::stoi(date.substr(5, 2)));
    const unsigned day = static_cast<unsigned>(std::stoi(date.substr(8, 2)));
    return daysFromCivil(year, month, day);
}

std::string addDays(const std::string& date, int days) {
    if (!isValidIsoDate(date)) {
        return date;
    }
    int year = 0;
    unsigned month = 0;
    unsigned day = 0;
    civilFromDays(toEpochDays(date) + days, year, month, day);
    char buffer[16];
    std::snprintf(buffer, sizeof(buffer), "%04d-%02u-%02u", year, month, day);
    return buffer;
}

int daysBetween(const std::string& from, const std::string& to) {
    if (!isValidIsoDate(from) || !isValidIsoDate(to)) {
        return 0;
    }
    return static_cast<int>(toEpochDays(to) - toEpochDays(from));
}

bool isPastDate(const std::string& date) {
    return isValidIsoDate(date) && date < todayIso();
}

bool isToday(const std::string& date) {
    return date == todayIso();
}

std::string weekdayName(const std::string& date) {
    static const std::array<const char*, 7> kNames{"Thursday", "Friday", "Saturday",
                                                   "Sunday",   "Monday", "Tuesday",
                                                   "Wednesday"};
    if (!isValidIsoDate(date)) {
        return {};
    }
    // 1970-01-01 was a Thursday.
    long index = toEpochDays(date) % 7;
    if (index < 0) {
        index += 7;
    }
    return kNames[static_cast<std::size_t>(index)];
}

} // namespace scapms::utils
