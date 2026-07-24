#pragma once
// ============================================================================
// ExchangeFilters — cache of Binance /api/v3/exchangeInfo per-symbol trading
// rules, and the normalizer that snaps every order to them BEFORE the gateway
// approves + submits it. (Phase-2 review fix item 5, 2026-07-11.)
//
// Binance rejects any order that violates LOT_SIZE (step/min/max qty),
// MARKET_LOT_SIZE (market-order qty step), MIN_NOTIONAL / NOTIONAL (min quote
// value) or price/qty precision. Before this, orders were sent raw — a sub-min
// or off-step qty would be rejected by the exchange (live) or silently accepted
// in shadow, masking a real defect.
//
// normalize() floors qty to the step, checks min/max qty + min-notional, and
// reports whether the order is submittable. Filters can be (re)loaded from a
// cached exchangeInfo JSON string so a filter CHANGE refreshes the cache and a
// now-sub-min order is rejected.
//
// CASE-INSENSITIVE KEYS (S-2026-07-23 -1013 root fix). Binance exchangeInfo
// returns symbols UPPERCASE ("SOLUSDT"), but the strategy books pass their
// symbol in whatever case the leg spec uses — the TrendRoster legs are LOWERCASE
// ("solusdt"/"btcusdt"). A raw std::map<string> lookup is case-sensitive, so a
// lowercase intent MISSED the uppercase cache entry: normalize() fell to the
// "no cached filter -> pass-through" branch and sent the RAW (unfloored) qty to
// Binance -> guaranteed -1013 "Filter failure: LOT_SIZE" (SOL 1.61260197,
// BTC 0.00211552 sent un-stepped). The backfill's has_valid() check UPPERCASED
// first, so it reported "0 still-missing" — a false-clean that hid the hot-path
// miss. Fix: every key is normalized to UPPER on both store and lookup (set/
// load_from_json store upper; has/get/has_valid/normalize upper their arg), so
// ANY caller case (trend + RSIrev + mimic + future legs) resolves to the one
// cached filter and gets floored. Uppercasing an already-upper symbol (the XSEC/
// RIP/mimic "BTC"+"USDT" callers) is a no-op, so working paths are unchanged.
//
// Header-only, no curl dependency — the JSON parse is a tiny hand-roll matching
// the repo's existing hand-rolled JSON style. The REST fetch is a thin
// live-activated method on BinanceREST (added separately); this cache is what
// the gateway consults on the hot path.
// ============================================================================
#include <string>
#include <map>
#include <cmath>
#include <cstdlib>
#include <cstdint>
#include <cctype>

namespace chimera {

struct SymbolFilter {
    double step_size    = 0.0;   // LOT_SIZE stepSize (0 => no constraint)
    double min_qty      = 0.0;   // LOT_SIZE minQty
    double max_qty      = 0.0;   // LOT_SIZE maxQty (0 => no cap)
    double market_step  = 0.0;   // MARKET_LOT_SIZE stepSize (0 => use step_size)
    double min_notional = 0.0;   // MIN_NOTIONAL / NOTIONAL minNotional
    double tick_size    = 0.0;   // PRICE_FILTER tickSize (0 => no constraint)
    int    qty_prec     = 8;     // baseAssetPrecision
    bool   valid        = false;
};

struct NormalizedOrder {
    double qty  = 0.0;
    bool   ok   = false;
    std::string reason;   // why rejected (empty on ok)
};

class ExchangeFilters {
public:
    // Install/refresh one symbol's rules (symbol case-insensitive, e.g. "BTCUSDT"
    // or "btcusdt" — stored + looked up UPPERCASE).
    void set(const std::string& symbol, SymbolFilter f) { f.valid = true; filters_[up(symbol)] = f; }

    bool has(const std::string& symbol) const { return filters_.count(up(symbol)) != 0; }
    size_t size() const { return filters_.size(); }

    const SymbolFilter* get(const std::string& symbol) const {
        auto it = filters_.find(up(symbol)); return it == filters_.end() ? nullptr : &it->second;
    }

    // S-2026-07-20: true iff a usable LOT_SIZE step is cached for this symbol.
    // Callers use this to make the normalize() pass-through LOUD on live buys.
    bool has_valid(const std::string& symbol) const {
        auto it = filters_.find(up(symbol));
        return it != filters_.end() && it->second.valid;
    }

    // Snap a MARKET order's qty to the applicable filters. If no cached filter
    // exists, pass through (can't over-constrain what we haven't fetched) but
    // still enforce a caller floor via the gateway's own min_notional.
    // ── FALLBACK LOT_SIZE STEP (2026-07-24, -1013 root belt-and-suspenders).
    //    The case-insensitive fix (S-2026-07-23) resolves the lowercase-intent miss,
    //    but if the exchangeInfo parse GAPS (truncated body, seen live: TIA/SAND/LINK,
    //    and the live SOL/BTC -1013s) a symbol has NO cached filter at all and
    //    normalize() used to pass RAW qty -> Binance -1013. This is the authoritative
    //    Binance-spot stepSize for the traded universe (fetched from api.binance.com,
    //    2026-07-24), used ONLY when no valid filter is cached, so qty is ALWAYS
    //    floored to a real step. A gap can no longer leak a raw un-stepped order.
    static double fallback_step(const std::string& up_sym) {
        static const std::map<std::string, double> S = {
            {"BTCUSDT", 0.00001}, {"ETHUSDT", 0.0001}, {"SOLUSDT", 0.001},
            {"XRPUSDT", 0.1},     {"XLMUSDT", 1.0},    {"ATOMUSDT", 0.01},
            {"DOTUSDT", 0.01},    {"DOGEUSDT", 1.0},   {"AVAXUSDT", 0.01},
            {"LINKUSDT", 0.01},   {"ADAUSDT", 0.1},    {"TRXUSDT", 0.1},
        };
        auto it = S.find(up_sym);
        return it == S.end() ? 0.0 : it->second;
    }

    // Snap a PRICE to the symbol's PRICE_FILTER tickSize, rounding DOWN to the
    // nearest tick. Binance rejects any order whose price is not a tickSize
    // multiple with -1013 PRICE_FILTER, which for a protective stop LOSES the
    // order. Rounding DOWN is the safe direction for a protective SELL: the stop
    // trigger and the (already-below-trigger) limit both move slightly lower, so
    // the stop stays protective and the limit stays below the trigger. If no
    // PRICE_FILTER is cached (unfetched/gapped exchangeInfo) the price passes
    // through unchanged — the caller's -1013 reject stays the loud backstop.
    double snap_price_to_tick(const std::string& symbol, double px) const {
        if (px <= 0.0) return px;
        auto it = filters_.find(up(symbol));
        if (it == filters_.end() || it->second.tick_size <= 0.0) return px;   // no tick -> pass-through
        double t = it->second.tick_size;
        double snapped = std::floor(px / t) * t;
        return round_prec(snapped, 8);   // kill FP residue so 8-dp formatting is exact
    }

    NormalizedOrder normalize(const std::string& symbol, double qty, double ref_px,
                              bool is_market = true) const {
        NormalizedOrder out; out.qty = qty;
        if (qty <= 0.0 || ref_px <= 0.0) { out.reason = "invalid qty/px"; return out; }
        auto it = filters_.find(up(symbol));
        if (it == filters_.end() || !it->second.valid) {
            // No cached filter — floor to the authoritative fallback step instead of
            // leaking RAW qty (the -1013 root). Unknown sym (no fallback) still passes
            // through, but the gateway logs that case LOUD via has_valid()==false.
            double fs = fallback_step(up(symbol));
            if (fs > 0.0) {
                double q = std::floor(qty / fs) * fs;
                q = round_prec(q, 8);
                if (q <= 0.0) { out.qty = 0; out.reason = "qty rounds to 0 (fallback step)"; return out; }
                out.qty = q; out.ok = true; return out;
            }
            out.ok = true; return out;
        }
        const SymbolFilter& fl = it->second;

        double step = is_market && fl.market_step > 0.0 ? fl.market_step : fl.step_size;
        double q = qty;
        if (step > 0.0) q = std::floor(q / step) * step;        // floor to the step
        q = round_prec(q, fl.qty_prec);

        if (q <= 0.0)                       { out.qty = 0; out.reason = "qty rounds to 0"; return out; }
        if (fl.min_qty > 0.0 && q < fl.min_qty) { out.qty = q; out.reason = "below LOT_SIZE minQty"; return out; }
        if (fl.max_qty > 0.0 && q > fl.max_qty) q = std::floor(fl.max_qty / (step > 0 ? step : 1e-8)) * (step > 0 ? step : 1);
        if (fl.min_notional > 0.0 && q * ref_px < fl.min_notional) { out.qty = q; out.reason = "below MIN_NOTIONAL"; return out; }

        out.qty = q; out.ok = true; return out;
    }

    // Parse an exchangeInfo JSON blob (Binance shape) and populate/refresh the
    // cache. Tolerant hand-roll: scans each {"symbol":"X", ... "filters":[...]}.
    // Returns number of symbols loaded. Existing entries are overwritten (refresh).
    int load_from_json(const std::string& body) {
        int n = 0; size_t pos = 0;
        const std::string sym_key = "\"symbol\":\"";
        while ((pos = body.find(sym_key, pos)) != std::string::npos) {
            size_t s = pos + sym_key.size();
            size_t e = body.find('"', s);
            if (e == std::string::npos) break;
            std::string symbol = body.substr(s, e - s);
            // bound this symbol's object at the next "symbol": key (or end)
            size_t next = body.find(sym_key, e);
            size_t obj_end = next == std::string::npos ? body.size() : next;
            std::string obj = body.substr(e, obj_end - e);

            SymbolFilter fl;
            fl.qty_prec     = (int)find_num(obj, "\"baseAssetPrecision\":", 8);
            fl.step_size    = find_filter_num(obj, "LOT_SIZE", "stepSize");
            fl.min_qty      = find_filter_num(obj, "LOT_SIZE", "minQty");
            fl.max_qty      = find_filter_num(obj, "LOT_SIZE", "maxQty");
            fl.market_step  = find_filter_num(obj, "MARKET_LOT_SIZE", "stepSize");
            double mn1      = find_filter_num(obj, "MIN_NOTIONAL", "minNotional");
            double mn2      = find_filter_num(obj, "NOTIONAL", "minNotional");
            fl.min_notional = mn1 > 0 ? mn1 : mn2;
            fl.tick_size    = find_filter_num(obj, "PRICE_FILTER", "tickSize");
            // S-2026-07-20 LOT_SIZE live-reject fix: a symbol whose object parsed WITHOUT
            // a LOT_SIZE stepSize (truncated/odd exchangeInfo body — seen live: TIA/SAND/
            // LINK raw-qty -1013 rejects while AVAX/SOL conformed, varying per fetch) used
            // to be stored valid=true with step 0 -> normalize applied NO floor and passed
            // raw qty to Binance. step==0 now means NOT valid -> normalize falls to the
            // explicit pass-through branch, which the gateway logs loudly on live BUYs,
            // and the mirror's -1013 retry (main.cpp) floors the qty itself.
            fl.valid        = fl.step_size > 0.0;
            filters_[up(symbol)] = fl; if (fl.valid) ++n;   // UPPER key: case-insensitive lookup (S-23)
            pos = obj_end;
        }
        return n;
    }

private:
    // Normalize a symbol to UPPERCASE for case-insensitive keying (S-2026-07-23).
    static std::string up(std::string s) {
        for (auto& c : s) c = (char)std::toupper((unsigned char)c);
        return s;
    }
    static double round_prec(double v, int prec) {
        if (prec < 0) prec = 0; if (prec > 12) prec = 12;
        double m = std::pow(10.0, prec);
        return std::floor(v * m + 0.5) / m;
    }
    static double find_num(const std::string& s, const std::string& key, double dflt) {
        size_t p = s.find(key); if (p == std::string::npos) return dflt;
        return std::strtod(s.c_str() + p + key.size(), nullptr);
    }
    // Find a numeric field inside the filter object of a given filterType.
    // Filters look like {"filterType":"LOT_SIZE","minQty":"0.001","stepSize":"0.001",...}
    static double find_filter_num(const std::string& obj, const char* ftype, const char* field) {
        std::string want = std::string("\"filterType\":\"") + ftype + "\"";
        size_t p = obj.find(want); if (p == std::string::npos) return 0.0;
        // scan within this filter object (until the next '}' after p)
        size_t close = obj.find('}', p); if (close == std::string::npos) close = obj.size();
        std::string seg = obj.substr(p, close - p);
        std::string fkey = std::string("\"") + field + "\":\"";
        size_t fp = seg.find(fkey);
        if (fp == std::string::npos) {   // some fields are unquoted numbers
            fkey = std::string("\"") + field + "\":";
            fp = seg.find(fkey); if (fp == std::string::npos) return 0.0;
            return std::strtod(seg.c_str() + fp + fkey.size(), nullptr);
        }
        return std::strtod(seg.c_str() + fp + fkey.size(), nullptr);
    }

    std::map<std::string, SymbolFilter> filters_;
};

} // namespace chimera
