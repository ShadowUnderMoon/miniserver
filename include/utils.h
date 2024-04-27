#pragma once
#include <string>


inline std::string EscapeString(const std::string& s) {
    std::string escaped;
    for (char c : s) {
        switch (c) {
            case '\n': escaped += "\\n"; break;
            case '\r': escaped += "\\r"; break;
            case '\t': escaped += "\\t"; break;
            case '\\': escaped += "\\\\"; break;
            case '\0': escaped += "\\0"; break;
            default: escaped += c; break;
        }
    }
    return escaped;
}