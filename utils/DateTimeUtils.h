#pragma once
#include <cctype>
#include <ctime>
#include <string>

namespace scapms::utils {

inline std::tm toLocalTm(std::time_t t) {
    std::tm out{};
#if defined(_MSC_VER)
    localtime_s(&out, &t);
#else
    out = *std::localtime(&t);
#endif
    return out;
}

inline std::string formatTime(std::time_t t, const char* format) {
    char buffer[40];
    std::tm tm = toLocalTm(t);
    std::strftime(buffer, sizeof buffer, format, &tm);
    return buffer;
}

inline std::string todayString() { return formatTime(std::time(nullptr), "%Y-%m-%d"); }
inline std::string nowString() { return formatTime(std::time(nullptr), "%Y-%m-%d %H:%M"); }
inline std::string clockString(std::time_t t) { return formatTime(t, "%H:%M:%S"); }

inline bool allDigits(const std::string& s, std::size_t from, std::size_t to) {
    for (std::size_t i = from; i < to; ++i)
        if (!std::isdigit(static_cast<unsigned char>(s[i]))) return false;
    return true;
}

// "yyyy-MM-dd"
inline bool isValidDate(const std::string& s) {
    if (s.size() != 10 || s[4] != '-' || s[7] != '-') return false;
    if (!allDigits(s, 0, 4) || !allDigits(s, 5, 7) || !allDigits(s, 8, 10)) return false;
    int y = std::stoi(s.substr(0, 4)), m = std::stoi(s.substr(5, 2)), d = std::stoi(s.substr(8, 2));
    if (y < 1900 || y > 2200 || m < 1 || m > 12 || d < 1) return false;
    static const int daysInMonth[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    int maxDay = daysInMonth[m - 1];
    bool leap = (y % 4 == 0 && y % 100 != 0) || (y % 400 == 0);
    if (m == 2 && leap) maxDay = 29;
    return d <= maxDay;
}

// "HH:mm"
inline bool isValidTime(const std::string& s) {
    if (s.size() != 5 || s[2] != ':' || !allDigits(s, 0, 2) || !allDigits(s, 3, 5)) return false;
    int h = std::stoi(s.substr(0, 2)), m = std::stoi(s.substr(3, 2));
    return h < 24 && m < 60;
}

// Returns -1 when the date or time is malformed.
inline std::time_t parseDateTime(const std::string& date, const std::string& time) {
    if (!isValidDate(date) || !isValidTime(time)) return static_cast<std::time_t>(-1);
    std::tm tm{};
    tm.tm_year = std::stoi(date.substr(0, 4)) - 1900;
    tm.tm_mon = std::stoi(date.substr(5, 2)) - 1;
    tm.tm_mday = std::stoi(date.substr(8, 2));
    tm.tm_hour = std::stoi(time.substr(0, 2));
    tm.tm_min = std::stoi(time.substr(3, 2));
    tm.tm_isdst = -1;
    return std::mktime(&tm);
}

inline std::string formatDuration(long seconds) {
    if (seconds < 0) seconds = 0;
    long h = seconds / 3600, m = (seconds % 3600) / 60, s = seconds % 60;
    if (h > 0) return std::to_string(h) + "h " + std::to_string(m) + "m";
    if (m > 0) return std::to_string(m) + "m " + std::to_string(s) + "s";
    return std::to_string(s) + "s";
}

} // namespace scapms::utils
