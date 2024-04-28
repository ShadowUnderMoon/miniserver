#pragma once
#include <regex>
#include <spdlog/spdlog.h>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <buffer.h>
#include <algorithm>
#include <http_constants.h>
class HttpRequest {
public:

    enum class REQUEST_STATE {
        REQUEST_LINE,
        REQUEST_HEADERS,
        REQUEST_BODY,
        REQUEST_FINISH,
    };

    enum class HTTP_CODE {
        NO_REQUEST,
        GET_REQUEST,
        BAD_REQUEST,
        NO_RESOURCE,
        FORBIDEN_REQUEST,
        FILE_REQUEST,
        INTERNAL_ERROR,
        CLOSED_CONNECTION,
    };

    HttpRequest() { Init(); }
    void Init() {
        method_ = path_ = version_ = body_ = "";
        state_ = REQUEST_STATE::REQUEST_LINE;
        header_.clear();
        post_.clear();
    }

    bool IsKeepAlive() const {
        if (header_.contains("Connection")) {
            return header_.at("Connection") == "keep-alive" && version_ == "1.1";
        }
        return false;
    }

    bool Parse(Buffer& buff) {
        const char CRLF[] = "\r\n";
        if (buff.ReadableBytes() <= 0) {
            return false;
        }

        while (buff.ReadableBytes() && state_ != REQUEST_STATE::REQUEST_FINISH) {
            const char* line_end = std::search(buff.Peak(), buff.BeginWrite(), CRLF, CRLF + 2);
            std::string line(buff.Peak(), line_end);
            SPDLOG_LOGGER_DEBUG(spdlog::get("miniserver"), line);
            switch (state_) {
            case REQUEST_STATE::REQUEST_LINE:
                if (!ParseRequestLine(line)) {
                    return false;
                }
                ParsePath();
                break;
            case REQUEST_STATE::REQUEST_HEADERS:
                ParseHeader(line);
                if (buff.ReadableBytes() <= 2) {
                    state_ = REQUEST_STATE::REQUEST_FINISH;
                }
                break;
            case REQUEST_STATE::REQUEST_BODY:
                ParseBody(line);
                break;
            default:
                break;
            }
            if (line_end == buff.BeginWrite()) {
                break;
            }
            buff.RetrieveUntil(line_end + 2);
        }
        
        return true;
    }


    void ParsePath() {
        if (path_ == "/") {
            path_ = "/index.html";
        } else {
            for (auto& item : DEFAULT_HTML) {
                if (item == path_) {
                    path_ += ".html";
                    break;
                }
            }
        }
    }
    
    // POST / HTTP/1.1
    bool ParseRequestLine(const std::string& line) {
        std::regex pattern("^([^ ]*) ([^ ]*) HTTP/([^ ]*)$");
        std::smatch sub_match;
        if (std::regex_match(line, sub_match, pattern)) {
            method_ = sub_match[1];
            path_ = sub_match[2];
            version_ = sub_match[3];
            state_ = REQUEST_STATE::REQUEST_HEADERS;
            return true;
        }
        SPDLOG_LOGGER_ERROR(spdlog::get("miniserver"), "RequestLine Error");
        return false;
    }

    void ParseHeader(const std::string& line) {
        std::regex pattern("^([^:]*): ?(.*)$");
        std::smatch sub_match;
        if (std::regex_match(line, sub_match, pattern)) {
            header_[sub_match[1]] = sub_match[2];
        } else {
            state_ = REQUEST_STATE::REQUEST_BODY;
        }
    }

    void ParseBody(const std::string& line) {
        body_ = line;
        // ParsePost();
        state_ = REQUEST_STATE::REQUEST_FINISH;
    }

    const std::string& path() const {
        return path_;
    }

    const std::string& method() const {
        return method_;
    }

    const std::string& version() const {
        return version_;
    }

private:

    REQUEST_STATE state_;
    std::string method_, path_, version_, body_;
    std::unordered_map<std::string, std::string> header_;
    std::unordered_map<std::string, std::string> post_;

    inline static const std::unordered_set<std::string> DEFAULT_HTML{
            "/index", "/register", "/login",
            "/welcome", "/video", "/picture",
    };
    inline static const std::unordered_map<std::string, int> DEFAULT_HTML_TAG {
            {"/register.html", 0}, {"/login.html", 1}
    };

};