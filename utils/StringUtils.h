#pragma once
#include <cctype>
#include <string>
#include <vector>

namespace scapms::utils {

inline std::string trim(const std::string& s) {
    std::size_t b = 0, e = s.size();
    while (b < e && std::isspace(static_cast<unsigned char>(s[b]))) ++b;
    while (e > b && std::isspace(static_cast<unsigned char>(s[e - 1]))) --e;
    return s.substr(b, e - b);
}

inline std::string toLower(std::string s) {
    for (auto& c : s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return s;
}

inline bool startsWithIgnoreCase(const std::string& text, const std::string& prefix) {
    return toLower(text).compare(0, prefix.size(), toLower(prefix)) == 0;
}

inline bool containsIgnoreCase(const std::string& text, const std::string& key) {
    return toLower(text).find(toLower(key)) != std::string::npos;
}

// Quote a CSV field when it contains a comma, quote or line break.
inline std::string csvEscape(const std::string& field) {
    if (field.find_first_of(",\"\n\r") == std::string::npos) return field;
    std::string out = "\"";
    for (char c : field) {
        if (c == '"') out += "\"\"";
        else out += c;
    }
    out += '"';
    return out;
}

// Split one CSV line into fields (handles quoted fields and doubled quotes).
inline std::vector<std::string> csvSplit(const std::string& line) {
    std::vector<std::string> fields;
    std::string current;
    bool inQuotes = false;
    for (std::size_t i = 0; i < line.size(); ++i) {
        char c = line[i];
        if (inQuotes) {
            if (c == '"') {
                if (i + 1 < line.size() && line[i + 1] == '"') { current += '"'; ++i; }
                else inQuotes = false;
            } else {
                current += c;
            }
        } else if (c == '"') {
            inQuotes = true;
        } else if (c == ',') {
            fields.push_back(current);
            current.clear();
        } else if (c != '\r') {
            current += c;
        }
    }
    fields.push_back(current);
    return fields;
}

} // namespace scapms::utils
