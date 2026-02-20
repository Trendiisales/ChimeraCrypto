#include "engine/InstitutionalEngine.hpp"
#include <iostream>
#include <cmath>
#include <chrono>
#include <iomanip>
#include <algorithm>

namespace chimera {

InstitutionalEngine::InstitutionalEngine(double starting_equity)
    : equity_(starting_equity),
      risk_governor_(starting_equity),
      capital_alloc_(3.0),
      exec_governor_(20.0, 15.0),
      shadow_executor_(true, risk_governor_, position_ledger_, order_tracker_)
{
    std::cout << "[ENGINE] Initialized | Equity=$" << starting_equity << "\n";
    
    shadow_log_.open("logs/chimera_shadow_orders.log", std::ios::app);
    if (shadow_log_.is_open()) {
        auto now = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);
        shadow_log_ << "\n=== TUNED SESSION: " 
                   << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S")
                   << " | Equity: $" << starting_equity 
                   << " | Cost Floor Enforcement: ACTIVE ===\n";
        shadow_log_.flush();
    }
}

InstitutionalEngine::~InstitutionalEngine() {
    if (shadow_log_.is_open()) {
        shadow_log_ << "=== End | Orders: " << order_tracker_.total_orders() 
                   << " | Blocked: " << blocked_orders_
                   << " | Total P&L: $" << pnl_tracker_.get_total_pnl() << " ===\n";
        shadow_log_.close();
    }
}

LastOrder InstitutionalEngine::get_last_order() const {
    std::lock_guard<std::mutex> lock(last_order_mutex_);
    return last_order_;
}

AdaptiveFadeController InstitutionalEngine::create_controller_for_symbol(const std::string& symbol) {
    std::string lower = symbol;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
    
    if (lower == "btcusdt") {
        return AdaptiveFadeController(0.85, 0.35, 6000, 6.5);
    } else if (lower == "ethusdt") {
        return AdaptiveFadeController(0.85, 0.35, 6000, 6.5);
    } else if (lower == "solusdt") {
        return AdaptiveFadeController(1.0, 0.4, 8000, 7.5);
    }
    return AdaptiveFadeController(0.85, 0.35, 6000, 6.5);
}

SymbolID InstitutionalEngine::string_to_symbol_id(const std::string& symbol) {
    std::string lower = symbol;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
    
    if (lower == "btcusdt") return SymbolID::BTC;
    if (lower == "ethusdt") return SymbolID::ETH;
    if (lower == "solusdt") return SymbolID::SOL;
    return SymbolID::BTC;
}

const char* InstitutionalEngine::get_governor_state() const
{
    if (latency_kill_.halted()) return "HALTED";
    if (risk_governor_.is_halted()) return "BLOCKED";
    if (!exec_governor_.allow_trading()) return "THROTTLED";
    return "ACTIVE";
}

void InstitutionalEngine::update_book(const std::string& symbol, double bid, double ask, double bid_size, double ask_size)
{
    auto& s = symbols_[symbol];
    
    // Initialize on first update
    if (s.last_mid == 0) {
        s.fade_controller = create_controller_for_symbol(symbol);
    }
    
    s.symbol_id = string_to_symbol_id(symbol);
    
    double mid = (bid + ask) * 0.5;
    double spread = ask - bid;
    double spread_bps = (spread / mid) * 10000.0;
    
    s.burst.record_tick(bid, ask, bid_size, ask_size);
    s.vol_alloc.record_price(mid);
    portfolio_alloc_.record_price(symbol, mid);
    
    s.imbalance.record(bid_size, ask_size);
    s.liquidity.record(bid_size, ask_size);
    
    pnl_tracker_.mark_to_market(s.symbol_id, mid);
    
    s.last_mid = mid;
    s.last_bid = bid;
    s.last_ask = ask;
    s.last_spread_bps = spread_bps;
}

void InstitutionalEngine::tick(const std::string& symbol)
{
    auto it = symbols_.find(symbol);
    if (it == symbols_.end()) return;
    
    auto& s = it->second;
    
    if (latency_kill_.halted()) return;
    if (s.burst.burst_detected()) return;
    if (!risk_governor_.allow_global()) return;
    if (!exec_governor_.allow_trading()) return;
    if (!throttle_.allow()) return;
    if (s.last_mid <= 0) return;
    
    // Build market context
    MarketContext ctx;
    ctx.spread_bps = s.last_spread_bps;
    ctx.volatility_bps = s.vol_alloc.volatility() * 10000.0;
    ctx.imbalance = s.imbalance.velocity_score();
    ctx.signal_raw = s.imbalance.velocity_score();
    ctx.estimated_slippage_bps = 2.0;
    ctx.regime_trending = false;
    ctx.regime_choppy = false;
    
    // Get trade decision from adaptive controller
    TradeDecision decision = s.fade_controller.evaluate(ctx, s.position_state);
    
    // Calculate conviction for logging
    double conviction = std::abs(ctx.signal_raw) * (1.0 + std::abs(ctx.imbalance));
    double cost_floor = (ctx.spread_bps + 6.5 + ctx.estimated_slippage_bps) * 1.5;
    
    // Check if trade blocked by cost floor
    if ((decision.enter_long || decision.enter_short) && conviction < cost_floor) {
        blocked_orders_++;
        
        std::cout << "[BLOCKED] " << symbol 
                 << " | Signal=" << std::fixed << std::setprecision(2) << ctx.signal_raw
                 << " | Conviction=" << conviction
                 << " | Cost Floor=" << cost_floor << " bps\n";
        return;
    }
    
    // Execute trade if decision made
    if (decision.enter_long || decision.enter_short || decision.exit_position) {
        
        bool is_buy = decision.enter_long;
        
        if (decision.exit_position) {
            is_buy = !s.position_state.is_long;
        }
        
        double vol_mult = s.vol_alloc.size_multiplier();
        double port_mult = portfolio_alloc_.weight(symbol);
        
        if (std::isnan(vol_mult) || vol_mult <= 0) vol_mult = 1.0;
        if (std::isnan(port_mult) || port_mult <= 0) port_mult = 0.33;
        
        double capital_per_symbol = equity_ * 0.1;
        double usd_size = capital_per_symbol * vol_mult * port_mult * decision.size_multiplier;
        double qty_size = usd_size / s.last_mid;
        
        if (qty_size <= 0 || std::isnan(qty_size)) return;
        
        // Get P&L before trade
        double pnl_before = pnl_tracker_.get_total_pnl();
        
        // Execute trade
        shadow_executor_.execute_impl(s.symbol_id, is_buy, s.last_mid, qty_size);
        
        // Update P&L tracker
        pnl_tracker_.update_trade(s.symbol_id, is_buy, qty_size, s.last_mid);
        pnl_tracker_.mark_to_market(s.symbol_id, s.last_mid);
        
        // Calculate P&L impact
        double pnl_after = pnl_tracker_.get_total_pnl();
        double pnl_impact = pnl_after - pnl_before;
        
        // Update position state
        if (decision.enter_long || decision.enter_short) {
            s.position_state.has_position = true;
            s.position_state.is_long = decision.enter_long;
            s.position_state.entry_price = s.last_mid;
            s.position_state.size = qty_size;
            s.position_state.entry_time = std::chrono::steady_clock::now();
        } else if (decision.exit_position) {
            s.position_state.has_position = false;
        }
        
        // Update last order
        {
            std::lock_guard<std::mutex> lock(last_order_mutex_);
            last_order_.symbol = symbol;
            last_order_.side = is_buy ? "BUY" : "SELL";
            last_order_.size = qty_size;
            last_order_.price = s.last_mid;
            last_order_.usd = usd_size;
            last_order_.signal = ctx.signal_raw;
            last_order_.conviction = conviction;
            last_order_.cost_floor = cost_floor;
            last_order_.pnl_impact = pnl_impact;
            
            auto now = std::chrono::system_clock::now();
            auto time = std::chrono::system_clock::to_time_t(now);
            char buf[16];
            strftime(buf, sizeof(buf), "%H:%M:%S", std::localtime(&time));
            last_order_.time = buf;
        }
        
        auto pos_pnl = pnl_tracker_.get_position_pnl(s.symbol_id);
        
        std::string action = decision.exit_position ? "EXIT" : "ENTER";
        
        if (shadow_log_.is_open()) {
            shadow_log_ << last_order_.time << " | "
                       << action << " | "
                       << symbol << " | "
                       << last_order_.side << " | "
                       << "Qty: " << std::fixed << std::setprecision(4) << qty_size << " | "
                       << "Price: $" << std::setprecision(2) << s.last_mid << " | "
                       << "Signal: " << std::setprecision(2) << ctx.signal_raw << " | "
                       << "Conviction: " << std::setprecision(1) << conviction << " bps | "
                       << "Cost Floor: " << cost_floor << " bps | "
                       << "P&L Impact: $" << std::setprecision(2) << pnl_impact << " | "
                       << "Total P&L: $" << pnl_after << "\n";
            shadow_log_.flush();
        }
        
        std::cout << "[" << action << "] " << symbol 
                 << " " << last_order_.side
                 << " " << std::fixed << std::setprecision(4) << qty_size 
                 << " @ $" << std::setprecision(2) << s.last_mid
                 << " | Conv=" << std::setprecision(1) << conviction 
                 << " > Floor=" << cost_floor
                 << " | P&L=$" << std::setprecision(2) << pnl_impact << "\n";
    }
}

double InstitutionalEngine::get_equity() const { return equity_; }

}
