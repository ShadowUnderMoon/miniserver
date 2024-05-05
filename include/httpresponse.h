#pragma once

#include "buffer.h"
#include <fcntl.h>
#include <spdlog/spdlog.h>
#include <string>
#include <string_view>
#include <sys/stat.h>
#include <sys/mman.h>
#include <unordered_map>
#include <http_constants.h>
#include <unistd.h>
#include <utils.h>
#include <logger.h>
class HttpResponse {
public:
    HttpResponse() {}

    ~HttpResponse() {
        UnmapFile();
    }

    void UnmapFile() {
        if (mm_file_) {
            munmap(mm_file_, mm_file_stat_.st_size);
            mm_file_ = nullptr;
        }
    }

    void Init(const std::string& src_dir, const std::string& path, bool is_keep_alive, int code) {
        if (mm_file_) {
            UnmapFile();
        }
        code_ = code;
        is_keep_alive_ = is_keep_alive;
        path_ = path;
        src_dir_ = src_dir;
        mm_file_ = nullptr;
        mm_file_stat_ = {};
    }

    void MakeResponse(Buffer& buff) {
        if (stat((src_dir_ + path_).data(), &mm_file_stat_) < 0 || S_ISDIR(mm_file_stat_.st_mode)) {
            code_ = 404;
        } else if (!(mm_file_stat_.st_mode & S_IROTH)) {
            code_ = 403;
        } else if (code_ == -1) {
            code_ = 200;
        }

        ErrorHtml();
        AddStatusLine(buff);
        AddHeader(buff);
        AddContent(buff);

    }

    void ErrorHtml() {
        if (ERRORCODE_PATH.contains(code_)) {
            SPDLOG_LOGGER_WARN(logger, "ERRORHTML");
            path_ = ERRORCODE_PATH.at(code_);
            stat((src_dir_ + path_).data(), &mm_file_stat_);
        }
    }

    void AddStatusLine(Buffer& buff) {
        std::string status_text{getHTTPStatusText(code_)};
        status_text = "HTTP/1.1 " + std::to_string(code_) + " "+ status_text + "\r\n";
        SPDLOG_LOGGER_DEBUG(logger, EscapeString(status_text));
        buff.Append(status_text);
    }

    void AddHeader(Buffer& buff) {
        std::string header;
        header += "Connection: ";
        if (is_keep_alive_) {
            header += "keep-alive\r\n";
            header += "keep-alive: max=6, timeout=120\r\n";
        } else {
            header += "close\r\n";
        }

        header += "Content-type: " + GetFileType() + "\r\n";
        SPDLOG_LOGGER_DEBUG(logger, EscapeString(header));
        buff.Append(header);
    }

    void AddContent(Buffer& buff) {
        int src_fd  = open((src_dir_ + path_).data(), O_RDONLY);
        if (src_fd < 0) {
            ErrorContent(buff, "File Not Found!");
            return;
        }

        int* mmret = (int*)mmap(0, mm_file_stat_.st_size, PROT_READ, MAP_PRIVATE, src_fd, 0);
        if (*mmret == -1) {
            close(src_fd);
            ErrorContent(buff, "File Not Found!");
            return;
        }
        mm_file_ = (char*) mmret;
        close(src_fd);
        std::string header = "Content-length: " + std::to_string(mm_file_stat_.st_size) + "\r\n\r\n"; 
        SPDLOG_LOGGER_DEBUG(logger, EscapeString(header));
        buff.Append(header);
    }
    std::string GetFileType() {
        auto idx = path_.find_last_of('.');
        if (idx == std::string::npos) {
            return "text/plain";
        }
        return GetContentTypeByExtension(path_.substr(idx));
    }

    char* File() {
        return mm_file_;
    }

    size_t FileLen() const {
        return mm_file_stat_.st_size;
    }

    void ErrorContent(Buffer& buff, std::string message) {
        std::string body;
        std::string status;
        body += "<html><title>Error</title>";
        body += "<body bgcolor=\"ffffff\">";
        status = getHTTPStatusText(code_);
        body += std::to_string(code_) + " : " + status + "\n";
        body += "<p>" + message + "</p>";
        body += "<hr><em>MiniServer</em></body></html>";
        buff.Append("Content-length: " + std::to_string(body.size()) + "\r\n\r\n");
        buff.Append(body);
    }
private:
    int code_ = -1;
    bool is_keep_alive_ = false;
    std::string path_;
    std::string src_dir_;
    char* mm_file_ = nullptr;
    struct stat mm_file_stat_{};

    inline static const std::unordered_map<int, std::string> ERRORCODE_PATH = {
        {400, "/400.html"},
        {403, "/403.html"},
        {404, "/404.html"}
    };
};