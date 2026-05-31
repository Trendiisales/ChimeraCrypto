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
