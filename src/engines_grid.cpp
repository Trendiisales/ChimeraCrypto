// S55: GRID sleeve (shadow) — maker-native passive market-making. Buy g% dips,
// sell g% rips, hold inventory through dips, sit out sustained bears via the macro
// 200d gate (g_macro_bull). Validated: +3-5%/yr macro-gated (more loose), uncorr,
// 21-25% maxDD. Ticked from the live feed callback. Shadow only.
static std::vector<chimera::GridEngine*> g_grids;

static void init_grids() {
    struct GS { const char* sym; const char* tag; };
    static const GS specs[] = {
        {"btcusdt","GRID-BTC"}, {"ethusdt","GRID-ETH"}, {"linkusdt","GRID-LINK"},
        {"solusdt","GRID-SOL"}, {"dogeusdt","GRID-DOGE"}, {"bnbusdt","GRID-BNB"},
        // S56 expansion: BTC-200dMA-gated continuous sim 2022->2026-06, both halves
        // positive, bounded DD (expansion_sweep.cpp): LTC +11.6%/yr DD12%,
        // FIL +9.3%/16%, UNI +9.2%/17%, ADA +6.3%/15%, XRP +5.9%/25%.
        {"ltcusdt","GRID-LTC"}, {"filusdt","GRID-FIL"}, {"uniusdt","GRID-UNI"},
        {"adausdt","GRID-ADA"}, {"xrpusdt","GRID-XRP"},
    };
    for (const auto& s : specs) {
        chimera::GridEngine::Config c;
        c.symbol = s.sym; c.tag = s.tag;
        c.grid_pct = 0.02; c.max_lots = 12; c.maker_fee = 0.0005; c.shadow = true;
        g_grids.push_back(new chimera::GridEngine(c));
    }
    std::printf("[GRID] initialised %zu shadow grid engines (g=2%%, 12 lots, macro-gated)\n", g_grids.size());
    std::fflush(stdout);
}

// S55: macro-bull base — bull-beta core (95% basket, hyst 3%, hard -20% DD). Shadow.
static chimera::MacroBaseEngine* g_macro_base = nullptr;
static void init_macro_base() {
    chimera::MacroBaseEngine::Config c;
    c.symbols = {"btcusdt","ethusdt","solusdt","bnbusdt","linkusdt"};
    c.alloc = 0.95; c.enter_band = 0.03; c.exit_band = 0.03; c.dd_stop = 0.20;
    g_macro_base = new chimera::MacroBaseEngine(c);
    std::printf("[MACRO-BASE] initialised: 5-asset equal-weight, 95%% alloc, hyst 3%%, hard-DD 20%% (shadow)\n");
    std::fflush(stdout);
}
