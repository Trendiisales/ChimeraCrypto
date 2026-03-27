#pragma once
// ============================================================================
// PositionJournal — Persist open positions to disk so they survive restart
// ============================================================================
// Writes data/positions.json on every entry and exit.
// On startup, BalancedEngine::restore_from_journal() reads this file back
// and re-instates any open positions so trailing stops and exits continue.
//
// Format (one JSON object per line):
//   {"sym":"ETH","id":1,"layer":"LIQ","is_long":true,
//    "entry_price":2450.12,"entry_ts":1711600000000,
//    "entered_qty":0.04120000,"peak_price":2461.33,
//    "mfe":4.58,"mae":-1.12,
//    "pyramid_done":false,"pyramid_qty":0,"pyramid_add_price":0,
//    "blended_entry":0,"total_qty":0.04120000}
//
// The file is REPLACED (not appended) on each write — always reflects
// current live state. On clean exit with no positions, file is emptied.
// ============================================================================
#include <string>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <cstdio>
#include <sys/stat.h>

namespace chimera {

struct JournaledPosition {
    std::string sym;           // "BTC" / "ETH" etc
    int         id  = -1;      // symbol index
    std::string layer;         // "LIQ" / "VWAP" / "MM-PRESSURE" etc
    bool        is_long        = true;
    double      entry_price    = 0.0;
    int64_t     entry_ts       = 0;
    double      entered_qty    = 0.0;
    double      peak_price     = 0.0;
    double      mfe            = 0.0;
    double      mae            = 0.0;
    bool        pyramid_done   = false;
    double      pyramid_qty    = 0.0;
    double      pyramid_add_price = 0.0;
    double      pyramid_peak_profit = 0.0;
    double      blended_entry  = 0.0;
    double      total_qty      = 0.0;
    bool        partial_exit_done = false;
};

class PositionJournal {
public:
    static constexpr const char* PATH = "data/positions.json";

    // Save current open positions. Call after every enter() and exit().
    // positions: array of at most MAX_SYMBOLS entries, only those with is_open=true.
    static void save(const std::vector<JournaledPosition>& positions) {
        // Ensure data dir exists
        { ::mkdir("data", 0755); }

        std::ofstream f(PATH, std::ios::trunc);
        if (!f.is_open()) {
            std::fprintf(stderr, "[JOURNAL] Cannot write %s\n", PATH);
            return;
        }
        for (const auto& p : positions) {
            f << std::fixed << std::setprecision(8)
              << "{\"sym\":\"" << p.sym << "\","
              << "\"id\":"     << p.id  << ","
              << "\"layer\":\"" << p.layer << "\","
              << "\"is_long\":" << (p.is_long ? "true" : "false") << ","
              << "\"entry_price\":" << p.entry_price << ","
              << "\"entry_ts\":"    << p.entry_ts    << ","
              << "\"entered_qty\":" << p.entered_qty << ","
              << "\"peak_price\":"  << p.peak_price  << ","
              << "\"mfe\":"         << std::setprecision(4) << p.mfe  << ","
              << "\"mae\":"         << p.mae  << ","
              << "\"pyramid_done\":" << (p.pyramid_done ? "true" : "false") << ","
              << std::setprecision(8)
              << "\"pyramid_qty\":"          << p.pyramid_qty          << ","
              << "\"pyramid_add_price\":"    << p.pyramid_add_price    << ","
              << "\"pyramid_peak_profit\":"  << std::setprecision(4) << p.pyramid_peak_profit << ","
              << std::setprecision(8)
              << "\"blended_entry\":"   << p.blended_entry   << ","
              << "\"total_qty\":"       << p.total_qty        << ","
              << "\"partial_exit_done\":" << (p.partial_exit_done ? "true" : "false")
              << "}\n";
        }
        f.flush();
    }

    // Load previously saved positions at startup.
    static std::vector<JournaledPosition> load() {
        std::vector<JournaledPosition> out;
        std::ifstream f(PATH);
        if (!f.is_open()) return out;

        std::string line;
        while (std::getline(f, line)) {
            if (line.size() < 10) continue;
            JournaledPosition p;
            p.sym          = ex_str(line, "sym");
            p.id           = (int)ex_dbl(line, "id");
            p.layer        = ex_str(line, "layer");
            p.is_long      = ex_bool(line, "is_long");
            p.entry_price  = ex_dbl(line, "entry_price");
            p.entry_ts     = (int64_t)ex_dbl(line, "entry_ts");
            p.entered_qty  = ex_dbl(line, "entered_qty");
            p.peak_price   = ex_dbl(line, "peak_price");
            p.mfe          = ex_dbl(line, "mfe");
            p.mae          = ex_dbl(line, "mae");
            p.pyramid_done         = ex_bool(line, "pyramid_done");
            p.pyramid_qty          = ex_dbl(line, "pyramid_qty");
            p.pyramid_add_price    = ex_dbl(line, "pyramid_add_price");
            p.pyramid_peak_profit  = ex_dbl(line, "pyramid_peak_profit");
            p.blended_entry        = ex_dbl(line, "blended_entry");
            p.total_qty            = ex_dbl(line, "total_qty");
            p.partial_exit_done    = ex_bool(line, "partial_exit_done");
            if (!p.sym.empty() && p.id >= 0 && p.entry_price > 0.0)
                out.push_back(p);
        }
        return out;
    }

    // Clear the journal (called on clean shutdown with no open positions)
    static void clear() {
        std::ofstream f(PATH, std::ios::trunc);
    }

private:
    static std::string ex_str(const std::string& s, const std::string& k) {
        auto pos = s.find("\"" + k + "\":\"");
        if (pos == std::string::npos) return "";
        pos += k.size() + 4;
        auto end = s.find('"', pos);
        return end != std::string::npos ? s.substr(pos, end - pos) : "";
    }
    static double ex_dbl(const std::string& s, const std::string& k) {
        auto pos = s.find("\"" + k + "\":");
        if (pos == std::string::npos) return 0.0;
        pos += k.size() + 3;
        // skip boolean values
        if (pos < s.size() && (s[pos] == 't' || s[pos] == 'f')) return 0.0;
        auto end = s.find_first_of(",}", pos);
        try { return std::stod(s.substr(pos, end - pos)); } catch (...) { return 0.0; }
    }
    static bool ex_bool(const std::string& s, const std::string& k) {
        auto pos = s.find("\"" + k + "\":");
        if (pos == std::string::npos) return false;
        pos += k.size() + 3;
        return s.substr(pos, 4) == "true";
    }
};

} // namespace chimera
