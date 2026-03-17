#pragma once

#include <chrono>
#include <cstdio>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <string>
#include <vector>

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
        if (resolved == path_ && f_ != nullptr) {
            reopen_if_missing_unlocked();
            prune_if_needed_unlocked(current_time_ms(), true);
            ensure_open_unlocked();
            return;
        }

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
        prune_if_needed_unlocked(current_time_ms(), true);
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
        reopen_if_missing_unlocked();
        prune_if_needed_unlocked(ts_ms);
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

    void reopen_if_missing_unlocked() {
        if (f_ == nullptr || path_.empty()) return;
        std::error_code ec;
        if (std::filesystem::exists(path_, ec)) return;
        std::fclose(f_);
        f_ = nullptr;
    }

    void prune_if_needed_unlocked(int64_t now_ms, bool force = false) {
        const int day_key = utc_day_key_from_ms(now_ms);
        if (!force && day_key == last_prune_day_key_) {
            return;
        }
        last_prune_day_key_ = day_key;
        prune_old_entries_unlocked(now_ms);
    }

    void prune_old_entries_unlocked(int64_t now_ms) {
        if (path_.empty()) return;

        if (f_ != nullptr) {
            std::fclose(f_);
            f_ = nullptr;
        }

        std::ifstream in(path_);
        if (!in.is_open()) {
            return;
        }

        const std::string cutoff = iso_utc_from_ms(now_ms - retention_window_ms_);
        std::vector<std::string> kept_lines;
        std::string line;
        bool trimmed = false;
        while (std::getline(in, line)) {
            if (line.empty()) continue;
            const std::string ts = extract_json_string_field(line, "ts");
            if (ts.empty() || ts >= cutoff) {
                kept_lines.push_back(line);
            } else {
                trimmed = true;
            }
        }
        in.close();

        if (!trimmed) {
            return;
        }

        const std::string tmp_path = path_ + ".tmp";
        {
            std::ofstream out(tmp_path, std::ios::trunc);
            for (const auto& kept : kept_lines) {
                out << kept << '\n';
            }
        }

        std::error_code ec;
        std::filesystem::rename(tmp_path, path_, ec);
        if (ec) {
            std::filesystem::remove(path_, ec);
            ec.clear();
            std::filesystem::rename(tmp_path, path_, ec);
            if (ec) {
                std::filesystem::remove(tmp_path, ec);
            }
        }
    }

    static std::string make_session_id() {
        const auto now = std::chrono::system_clock::now();
        const auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()).count();
        return "sess-" + std::to_string((long long)now_ms);
    }

    static int64_t current_time_ms() {
        const auto now = std::chrono::system_clock::now();
        return std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()).count();
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

    static int utc_day_key_from_ms(int64_t ts_ms) {
        std::time_t secs = static_cast<std::time_t>(ts_ms / 1000);
        std::tm tm_buf{};
        gmtime_r(&secs, &tm_buf);
        return (tm_buf.tm_year + 1900) * 1000 + tm_buf.tm_yday;
    }

    static std::string extract_json_string_field(const std::string& line, const std::string& key) {
        const std::string needle = "\"" + key + "\":\"";
        auto pos = line.find(needle);
        if (pos == std::string::npos) return std::string();
        pos += needle.size();
        auto end = line.find('"', pos);
        if (end == std::string::npos) return std::string();
        return line.substr(pos, end - pos);
    }

    static constexpr const char* default_path_ = "data/execution_audit.jsonl";
    static constexpr int retention_days_ = 7;
    static constexpr int64_t retention_window_ms_ =
        static_cast<int64_t>(retention_days_) * 24LL * 60LL * 60LL * 1000LL;

    FILE* f_ = nullptr;
    std::string path_;
    std::string session_id_;
    int last_prune_day_key_ = -1;
    std::mutex mtx_;
};

} // namespace chimera
