#pragma once

#include <fstream>
#include <sstream>
#include <string>

namespace chimera {

struct RuntimeConfig {
    bool loaded = false;
    std::string source_path = "config/live_config.json";
    std::string credentials_file = "config/binance_credentials.json";
    bool shadow_mode = true;
    bool shadow_mode_set = false;
    bool paper_research_enabled = false;
    std::string audit_log_file = "data/execution_audit.jsonl";

    static RuntimeConfig load(const std::string& path = "config/live_config.json") {
        RuntimeConfig cfg;
        cfg.source_path = path;

        std::ifstream f(path);
        if (!f.is_open()) {
            return cfg;
        }

        std::string content((std::istreambuf_iterator<char>(f)),
                            std::istreambuf_iterator<char>());

        const std::string credentials = extract_json_string(content, "credentials_file");
        if (!credentials.empty()) {
            cfg.credentials_file = credentials;
        }

        if (extract_json_bool(content, "shadow_mode", cfg.shadow_mode)) {
            cfg.shadow_mode_set = true;
        }

        bool paper_research = false;
        if (extract_json_bool(content, "paper_research_enabled", paper_research)) {
            cfg.paper_research_enabled = paper_research;
        }

        const std::string audit_log = extract_json_string(content, "audit_log_file");
        if (!audit_log.empty()) {
            cfg.audit_log_file = audit_log;
        }

        cfg.loaded = true;
        return cfg;
    }

private:
    static std::string extract_json_string(const std::string& content, const std::string& key) {
        const std::string needle = "\"" + key + "\"";
        auto pos = content.find(needle);
        if (pos == std::string::npos) return "";

        auto colon = content.find(':', pos + needle.size());
        if (colon == std::string::npos) return "";

        auto first_quote = content.find('"', colon + 1);
        if (first_quote == std::string::npos) return "";

        auto end_quote = first_quote + 1;
        while (end_quote < content.size()) {
            end_quote = content.find('"', end_quote);
            if (end_quote == std::string::npos) return "";
            if (end_quote == first_quote + 1 || content[end_quote - 1] != '\\') break;
            ++end_quote;
        }
        if (end_quote == std::string::npos || end_quote <= first_quote) return "";
        return content.substr(first_quote + 1, end_quote - first_quote - 1);
    }

    static bool extract_json_bool(const std::string& content, const std::string& key, bool& out) {
        const std::string needle = "\"" + key + "\"";
        auto pos = content.find(needle);
        if (pos == std::string::npos) return false;

        auto colon = content.find(':', pos + needle.size());
        if (colon == std::string::npos) return false;

        auto value_start = content.find_first_not_of(" \t\r\n", colon + 1);
        if (value_start == std::string::npos) return false;

        if (content.compare(value_start, 4, "true") == 0) {
            out = true;
            return true;
        }
        if (content.compare(value_start, 5, "false") == 0) {
            out = false;
            return true;
        }
        return false;
    }
};

} // namespace chimera
