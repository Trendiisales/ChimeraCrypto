#pragma once

#include <chrono>
#include <cstdio>
#include <ctime>
#include <filesystem>
#include <mutex>
#include <string>

namespace chimera {

class ExecutionAuditLogger {
public:
    static ExecutionAuditLogger& instance() {
        static ExecutionAuditLogger logger;
        return logger;
    }

    void configure(const std::string& path) {
        std::lock_guard<std::mutex> lk(mtx_);
        const std::string resolved = path.empty() ? default_path_ : path;
        if (resolved == path_ && f_ != nullptr) return;

        if (f_ != nullptr) {
            std::fclose(f_);
            f_ = nullptr;
        }

        path_ = resolved;
        std::error_code ec;
        std::filesystem::path fs_path(path_);
        if (fs_path.has_parent_path()) {
            std::filesystem::create_directories(fs_path.parent_path(), ec);
        }
        f_ = std::fopen(path_.c_str(), "a");
    }

    void record_now(const std::string& event, const std::string& fields = std::string()) {
        const auto now = std::chrono::system_clock::now();
        const auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()).count();
        record_at(now_ms, event, fields);
    }

    void record_at(int64_t ts_ms, const std::string& event, const std::string& fields = std::string()) {
        std::lock_guard<std::mutex> lk(mtx_);
        ensure_open_unlocked();
        if (f_ == nullptr) return;

        std::fprintf(f_,
                     "{\"ts\":\"%s\",\"ts_ms\":%lld,\"session_id\":\"%s\",\"event\":\"%s\"%s%s}\n",
                     iso_utc_from_ms(ts_ms).c_str(),
                     (long long)ts_ms,
                     session_id_.c_str(),
                     escape_json(event).c_str(),
                     fields.empty() ? "" : ",",
                     fields.c_str());
        std::fflush(f_);
    }

    const std::string& path() const { return path_; }
    const std::string& session_id() const { return session_id_; }

    static std::string escape_json(const std::string& in) {
        std::string out;
        out.reserve(in.size() + 8);
        for (char c : in) {
            switch (c) {
                case '\\': out += "\\\\"; break;
                case '"':  out += "\\\""; break;
                case '\n': out += "\\n"; break;
                case '\r': out += "\\r"; break;
                case '\t': out += "\\t"; break;
                default:   out += c; break;
            }
        }
        return out;
    }

private:
    ExecutionAuditLogger()
        : path_(default_path_),
          session_id_(make_session_id()) {}

    ~ExecutionAuditLogger() {
        if (f_ != nullptr) {
            std::fclose(f_);
            f_ = nullptr;
        }
    }

    void ensure_open_unlocked() {
        if (f_ != nullptr) return;
        std::error_code ec;
        std::filesystem::path fs_path(path_);
        if (fs_path.has_parent_path()) {
            std::filesystem::create_directories(fs_path.parent_path(), ec);
        }
        f_ = std::fopen(path_.c_str(), "a");
    }

    static std::string make_session_id() {
        const auto now = std::chrono::system_clock::now();
        const auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()).count();
        return "sess-" + std::to_string((long long)now_ms);
    }

    static std::string iso_utc_from_ms(int64_t ts_ms) {
        std::time_t secs = static_cast<std::time_t>(ts_ms / 1000);
        std::tm tm_buf{};
        gmtime_r(&secs, &tm_buf);

        char buf[40];
        std::snprintf(buf, sizeof(buf),
                      "%04d-%02d-%02dT%02d:%02d:%02d.%03lldZ",
                      tm_buf.tm_year + 1900,
                      tm_buf.tm_mon + 1,
                      tm_buf.tm_mday,
                      tm_buf.tm_hour,
                      tm_buf.tm_min,
                      tm_buf.tm_sec,
                      (long long)(ts_ms % 1000));
        return std::string(buf);
    }

    static constexpr const char* default_path_ = "data/execution_audit.jsonl";

    FILE* f_ = nullptr;
    std::string path_;
    std::string session_id_;
    std::mutex mtx_;
};

} // namespace chimera
