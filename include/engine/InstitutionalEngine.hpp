#pragma once
#include <unordered_map>
#include <string>
#include <fstream>
#include <mutex>
#include <chrono>
#include "core/Portfolio.hpp"
#include "core/PositionLedger.hpp"
#include "core/OrderTracker.hpp"
#include "core/PnLTracker.hpp"
#include "core/Symbol.hpp"
#include "market/MicroburstDetector.hpp"
#include "risk/VolatilityAllocator.hpp"
#include "risk/PortfolioAllocator.hpp"
#include "risk/RiskGovernor.hpp"
#include "risk/CapitalAllocator.hpp"
#include "alpha/ImbalanceVelocity.hpp"
#include "alpha/LiquidityVacuum.hpp"
#include "alpha/ShockAlpha.hpp"
#include "alpha/SpreadCompression.hpp"
#include "execution/LatencyKillSwitch.hpp"
#include "execution/ExecutionGovernor.hpp"
#include "execution/AdaptiveThrottle.hpp"
#include "live/ShadowSpotExecutor.hpp"
#include "engine/AdaptiveFadeController.hpp"

namespace chimera {

struct LastOrder {
    std::string symbol;
    std::string side;
    double size;
    double price;
    double usd;
    double signal;
    double conviction;
    double cost_floor;
    std::string time;
    double pnl_impact;
};

class InstitutionalEngine {
public:
    explicit InstitutionalEngine(double starting_equity);
    ~InstitutionalEngine();

    void update_book(const std::string& symbol, double bid, double ask, double bid_size, double ask_size);
    void tick(const std::string& symbol);

    double get_equity() const;
    bool is_halted() const { return latency_kill_.halted(); }
    const char* get_governor_state() const;
    
    int total_shadow_orders() const { return order_tracker_.total_orders(); }
    int num_positions() const { return position_ledger_.num_positions(); }
    int get_blocked_orders() const { return blocked_orders_; }
    
    LastOrder get_last_order() const;
    double get_position(SymbolID id) const { return position_ledger_.get_position(id); }
    
    double get_realized_pnl() const { return pnl_tracker_.get_total_realized_pnl(); }
    double get_unrealized_pnl() const { return pnl_tracker_.get_total_unrealized_pnl(); }
    double get_total_pnl() const { return pnl_tracker_.get_total_pnl(); }

private:
    double equity_;
    int blocked_orders_ = 0;
    
    RiskGovernor risk_governor_;
    CapitalAllocator capital_alloc_;
    PortfolioAllocator portfolio_alloc_;
    
    LatencyKillSwitch latency_kill_;
    ExecutionGovernor exec_governor_;
    AdaptiveThrottle throttle_;
    
    PositionLedger position_ledger_;
    OrderTracker order_tracker_;
    PnLTracker pnl_tracker_;
    ShadowSpotExecutor shadow_executor_;
    
    std::ofstream shadow_log_;
    
    mutable std::mutex last_order_mutex_;
    LastOrder last_order_;

    struct SymbolState {
        Portfolio portfolio;
        MicroburstDetector burst;
        VolatilityAllocator vol_alloc{0.02};
        
        ImbalanceVelocity imbalance;
        LiquidityVacuum liquidity;
        ShockAlpha shock;
        SpreadCompression spread;
        
        AdaptiveFadeController fade_controller;
        PositionState position_state;
        
        double last_mid = 0.0;
        double last_bid = 0.0;
        double last_ask = 0.0;
        double last_spread_bps = 0.0;
        
        SymbolID symbol_id = SymbolID::BTC;
        
        SymbolState() : fade_controller(0.85, 0.35, 6000, 6.5) {}
    };

    std::unordered_map<std::string, SymbolState> symbols_;
    
    SymbolID string_to_symbol_id(const std::string& symbol);
    AdaptiveFadeController create_controller_for_symbol(const std::string& symbol);
};

}
