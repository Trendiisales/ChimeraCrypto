#pragma once
// ============================================================================
// NGASLeadLagEngine.hpp — Natural Gas → Crypto macro lead-lag signal
// ============================================================================
//
// SIGNAL LOGIC:
//   1. NGASFetcher detects NGAS price change exceeds threshold (±2% / 15min)
//   2. We check: is this session tradeable? (EU open or US open only)
//   3. We check: has the crypto market NOT already priced this in?
//      (crypto_move_bp < CRYPTO_ALREADY_MOVED_MAX_BP)
//   4. We check: cooldown window not active
//   5. Direction:
//      - NGAS spike UP   → SHORT is the natural call (risk-off) — but we are
//        SPOT-ONLY (no shorting). So spike up = SKIP or wait for reversal.
//      - NGAS drop DOWN  → LONG crypto (risk-on rotation into BTC/ETH)
//
// SCOPE: BTC (id=0) and ETH (id=1) only — most correlated to macro.
//        SOL/BNB/etc. correlation to NGAS is much weaker.
//
// SIZING: slow-burn signal like FUNDING — eng_mult=2.0, wider SL/TP.
//
// PARAMETERS: all in TradingConfig::NGAS_* constants
// ============================================================================

#pragma once
#include <cstdint>
#include <cmath>
#include <array>
#include <cstdio>
#include "core/SymbolIndex.hpp"
#include "core/market_data/NGASFetcher.hpp"
#include "config/TradingConfig.hpp"

namespace chimera {

class NGASLeadLagEngine {
public:
    NGASLeadLagEngine() {
        last_entry_ts_.fill(0);
        crypto_price_at_signal_.fill(0.0);
    }

    // ── Called from BalancedEngine::try_ngas_entry() ─────────────────────────

    // Returns true if a valid NGAS→crypto LONG signal exists for this symbol.
    // (SHORT not available — spot-only system)
    //
    // crypto_price  : current spot price of the symbol
    // now_ms        : current timestamp in milliseconds
    // latency_ms    : current WS latency (gate: not latency-sensitive, use loose threshold)
    bool check_long_signal(int id, double crypto_price, int64_t now_ms, double latency_ms) {
        if (!valid_symbol(id))   return false;
        if (!fetcher_)           return false;
        if (!fetcher_->ready())  return false;

        // Only fire on NGAS risk-on drop (dir = -1)
        int dir = fetcher_->signal_dir();
        if (dir != -1) return false;

        // Cooldown — macro signal is slow, no stacking
        if (now_ms - last_entry_ts_[id] < TradingConfig::NGAS_COOLDOWN_MS) return false;

        // Latency gate (loose — macro signal, not latency-sensitive)
        if (latency_ms > TradingConfig::NGAS_LATENCY_MAX_MS) return false;

        // Don't enter if crypto has already moved too much (signal absorbed)
        if (crypto_price_at_signal_[id] > 0.0) {
            double already_moved_bp = (crypto_price - crypto_price_at_signal_[id])
                                      / crypto_price_at_signal_[id] * 10000.0;
            if (already_moved_bp >= TradingConfig::NGAS_CRYPTO_MOVED_MAX_BP) {
                std::printf("[NGAS-ENGINE] %s crypto already moved %.2fbp — too late\n",
                    sym_short(id), already_moved_bp);
                std::fflush(stdout);
                clear_signal(id);
                return false;
            }
        } else {
            // First check after NGAS signal — record current crypto price as baseline
            crypto_price_at_signal_[id] = crypto_price;
        }

        double ngas_chg = fetcher_->change_pct();
        double ngas_px  = fetcher_->price();

        std::printf("[NGAS-ENGINE] %s | ngas=%.4f | chg=%.2f%% | crypto=%.4f | LONG signal valid\n",
            sym_short(id), ngas_px, ngas_chg, crypto_price);
        std::fflush(stdout);

        return true;
    }

    // Called when BalancedEngine successfully enters — consumes signal + sets cooldown
    void consume_signal(int id, int64_t now_ms) {
        if (!valid_symbol(id)) return;
        last_entry_ts_[id] = now_ms;
        clear_signal(id);
    }

    // Called each tick to arm the signal baseline when NGAS event first detected
    // (so we can measure how much crypto has moved since the NGAS move)
    void arm_if_new_signal(int id, double crypto_price) {
        if (!fetcher_ || !fetcher_->ready()) return;
        if (!valid_symbol(id)) return;
        int dir = fetcher_->signal_dir();
        if (dir != -1) {
            // Signal direction changed or gone — reset baseline
            clear_signal(id);
            return;
        }
        // Don't overwrite an existing baseline
        if (crypto_price_at_signal_[id] <= 0.0) {
            crypto_price_at_signal_[id] = crypto_price;
        }
    }

    void set_fetcher(NGASFetcher* f)  { fetcher_ = f; }
    NGASFetcher* fetcher()            { return fetcher_; }

    double ngas_price()      const { return fetcher_ ? fetcher_->price()      : 0.0; }
    double ngas_change_pct() const { return fetcher_ ? fetcher_->change_pct() : 0.0; }
    int    ngas_signal_dir() const { return fetcher_ ? fetcher_->signal_dir() : 0;   }

private:
    NGASFetcher* fetcher_ = nullptr;

    std::array<int64_t, MAX_SYMBOLS> last_entry_ts_;
    std::array<double,  MAX_SYMBOLS> crypto_price_at_signal_;

    static bool valid_symbol(int id) {
        // Only BTC (0) and ETH (1) — strongest macro correlation
        return (id == 0 || id == 1);
    }

    void clear_signal(int id) {
        crypto_price_at_signal_[id] = 0.0;
    }
};

} // namespace chimera
