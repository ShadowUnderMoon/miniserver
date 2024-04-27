#pragma once
#include <stdexcept>
#include <string>
#include <string_view>
#include <algorithm>

inline std::string_view getHTTPStatusText(int status) {
    using namespace std::string_view_literals;
    static constexpr std::pair<int, std::string_view> kStatusMapping[] = {
        {100, "Continue"sv},
        {101, "Switching Protocols"sv},
        {102, "Processing"sv},

        {200, "OK"sv},
        {201, "Created"sv},
        {202, "Accepted"sv},
        {203, "Non-Authoritative Information"sv},
        {204, "No Content"sv},
        {205, "Reset Content"sv},
        {206, "Partial Content"sv},
        {207, "Multi-Status"sv},
        {208, "Already Reported"sv},
        {226, "IM Used"sv},

        {300, "Multiple Choices"sv},
        {301, "Moved Permanently"sv},
        {302, "Found"sv},
        {303, "See Other"sv},
        {304, "Not Modified"sv},
        {305, "Use Proxy"sv},
        {306, "Switch Proxy"sv},
        {307, "Temporary Redirect"sv},
        {308, "Permanent Redirect"sv},

        {400, "Bad Request"sv},
        {401, "Unauthorized"sv},
        {402, "Payment Required"sv},
        {403, "Forbidden"sv},
        {404, "Not Found"sv},
        {405, "Method Not Allowed"sv},
        {406, "Not Acceptable"sv},
        {407, "Proxy Authentication Required"sv},
        {408, "Request Timeout"sv},
        {409, "Conflict"sv},
        {410, "Gone"sv},
        {411, "Length Required"sv},
        {412, "Precondition Failed"sv},
        {413, "Payload Too Large"sv},
        {414, "URI Too Long"sv},
        {415, "Unsupported Media Type"sv},
        {416, "Range Not Satisfiable"sv},
        {417, "Expectation Failed"sv},
        {418, "I'm a teapot"sv},
        {421, "Misdirected Request"sv},
        {422, "Unprocessable Entity"sv},
        {423, "Locked"sv},
        {424, "Failed Dependency"sv},
        {426, "Upgrade Required"sv},
        {428, "Precondition Required"sv},
        {429, "Too Many Requests"sv},
        {431, "Request Header Fields Too Large"sv},
        {451, "Unavailable For Legal Reasons"sv},

        {500, "Internal Server Error"sv},
        {501, "Not Implemented"sv},
        {502, "Bad Gateway"sv},
        {503, "Service Unavailable"sv},
        {504, "Gateway Timeout"sv},
        {505, "HTTP Version Not Supported"sv},
        {506, "Variant Also Negotiates"sv},
        {507, "Insufficient Storage"sv},
        {508, "Loop Detected"sv},
        {510, "Not Extended"sv},
        {511, "Network Authentication Required"sv},
    };
    if (status == 200) { return "OK"sv; }
    auto it = std::lower_bound(
        std::begin(kStatusMapping), std::end(kStatusMapping), status,
        [](auto const &p, auto status) { return p.first < status; });
    if (it == std::end(kStatusMapping) || it->first != status) [[unlikely]] {
        throw std::invalid_argument("status code not recognized: " + std::to_string(status));
    } else {
        return it->second;
    }
}

inline std::string GetContentTypeByExtension(std::string_view ext) {
    using namespace std::string_view_literals;
    using namespace std::string_literals;
    if (ext == ".html"sv || ext == ".htm"sv) {
        return "text/html;charset=utf-8"s;
    } else if (ext == ".css"sv) {
        return "text/css;charset=utf-8"s;
    } else if (ext == ".js"sv) {
        return "application/javascript;charset=utf-8"s;
    } else if (ext == ".txt"sv || ext == ".md"sv) {
        return "text/plain;charset=utf-8"s;
    } else if (ext == ".json"sv) {
        return "application/json"s;
    } else if (ext == ".png"sv) {
        return "image/png"s;
    } else if (ext == ".jpg"sv || ext == ".jpeg"sv) {
        return "image/jpeg"s;
    } else if (ext == ".gif"sv) {
        return "image/gif"s;
    } else if (ext == ".xml"sv) {
        return "application/xml"s;
    } else if (ext == ".pdf"sv) {
        return "application/pdf"s;
    } else if (ext == ".mp4"sv) {
        return "video/mp4"s;
    } else if (ext == ".mp3"sv) {
        return "audio/mp3"s;
    } else if (ext == ".zip"sv) {
        return "application/zip"s;
    } else if (ext == ".svg"sv) {
        return "image/svg+xml"s;
    } else if (ext == ".wav"sv) {
        return "audio/wav"s;
    } else if (ext == ".ogg"sv) {
        return "audio/ogg"s;
    } else if (ext == ".mpg"sv || ext == ".mpeg"sv) {
        return "video/mpeg"s;
    } else if (ext == ".webm"sv) {
        return "video/webm"s;
    } else if (ext == ".ico"sv) {
        return "image/x-icon"s;
    } else if (ext == ".rar"sv) {
        return "application/x-rar-compressed"s;
    } else if (ext == ".7z"sv) {
        return "application/x-7z-compressed"s;
    } else if (ext == ".tar"sv) {
        return "application/x-tar"s;
    } else if (ext == ".gz"sv) {
        return "application/gzip"s;
    } else if (ext == ".bz2"sv) {
        return "application/x-bzip2"s;
    } else if (ext == ".xz"sv) {
        return "application/x-xz"s;
    } else if (ext == ".zip"sv) {
        return "application/zip"s;
    } else if (ext == ".tar.gz"sv || ext == ".tgz"sv) {
        return "application/tar+gzip"s;
    } else if (ext == ".tar.bz2"sv || ext == ".tbz2"sv) {
        return "application/tar+bzip2"s;
    } else if (ext == ".tar.xz"sv || ext == ".txz"sv) {
        return "application/tar+xz"s;
    } else if (ext == ".doc"sv || ext == ".docx"sv) {
        return "application/msword"s;
    } else if (ext == ".xls"sv || ext == ".xlsx"sv) {
        return "application/vnd.ms-excel"s;
    } else if (ext == ".ppt"sv || ext == ".pptx"sv) {
        return "application/vnd.ms-powerpoint"s;
    } else if (ext == ".csv"sv) {
        return "text/csv;charset=utf-8"s;
    } else if (ext == ".rtf"sv) {
        return "application/rtf"s;
    } else if (ext == ".exe"sv) {
        return "application/x-msdownload"s;
    } else if (ext == ".msi"sv) {
        return "application/x-msi"s;
    } else if (ext == ".bin"sv) {
        return "application/octet-stream"s;
    } else {
        throw std::invalid_argument("extension is not recognized: "s + std::string(ext));
    }
}