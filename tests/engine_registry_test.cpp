// Phase-4 item 20 — engine registry / wiring reconciliation.
// Tests: (a) registry says LIVE but engine not wired -> validate() fails
// (startup would abort); (b) DISABLED but actually connected -> fails
// (inverse overstatement); (c) connected_count() == real connected instances;
// (d) a healthy declaration passes.
#include "live/EngineRegistry.hpp"
#include <cstdio>
#include <cassert>

using namespace chimera;

int main() {
    int failures = 0;
    auto check = [&](bool cond, const char* msg) {
        if (!cond) { std::printf("  FAIL: %s\n", msg); failures++; }
    };

    // (a) declared LIVE, never wired -> abort
    {
        EngineRegistry reg;
        reg.declare("XSEC-BTC", Lifecycle::LIVE);
        reg.declare("RIPRIDER", Lifecycle::SHADOW);
        reg.mark_wired("RIPRIDER", /*connected*/true, 1);
        // XSEC-BTC declared LIVE but never marked wired
        std::string err;
        bool ok = reg.validate(err);
        check(!ok, "LIVE-but-unwired must fail validate()");
        check(err.find("XSEC-BTC") != std::string::npos, "err names the offender");
        check(err.find("NOT WIRED") != std::string::npos, "err says not wired");
        std::printf("  (a) LIVE unwired -> reject: %s\n", err.c_str());
    }

    // (a2) declared SHADOW, wired but callback disconnected -> abort
    {
        EngineRegistry reg;
        reg.declare("EDGE", Lifecycle::SHADOW);
        reg.mark_wired("EDGE", /*connected*/false, 10);
        std::string err;
        check(!reg.validate(err), "wired-but-disconnected active must fail");
        check(err.find("DISCONNECTED") != std::string::npos, "err says disconnected");
        std::printf("  (a2) SHADOW disconnected -> reject: %s\n", err.c_str());
    }

    // (b) declared DISABLED but actually connected -> abort (inverse)
    {
        EngineRegistry reg;
        reg.declare("LEGACY-EDGE", Lifecycle::DISABLED);
        reg.mark_wired("LEGACY-EDGE", /*connected*/true, 285);
        std::string err;
        check(!reg.validate(err), "DISABLED-but-connected must fail");
        check(err.find("DISABLED but is actually CONNECTED") != std::string::npos,
              "err flags the phantom-running engine");
        std::printf("  (b) DISABLED but running -> reject: %s\n", err.c_str());
    }

    // (c)+(d) a healthy graph passes and the count is REAL (not aspirational)
    {
        EngineRegistry reg;
        reg.declare("XSEC-BTC",  Lifecycle::SHADOW);
        reg.declare("XSEC-BR",   Lifecycle::SHADOW);
        reg.declare("RIPRIDER",  Lifecycle::SHADOW);
        reg.declare("UPJUMP-GRID", Lifecycle::SHADOW);
        reg.declare("LEGACY-EDGE", Lifecycle::DISABLED);   // off, and not wired
        reg.declare("FUTURES-LIQ", Lifecycle::HALTED);     // operational, no assert
        reg.mark_wired("XSEC-BTC", true, 1);
        reg.mark_wired("XSEC-BR",  true, 1);
        reg.mark_wired("RIPRIDER", true, 1);
        reg.mark_wired("UPJUMP-GRID", true, 32);           // 32 grid cells
        std::string err;
        bool ok = reg.validate(err);
        check(ok, "healthy declaration must pass validate()");
        // connected_count = 1+1+1+32 = 35, NOT a hardcoded banner number
        int cc = reg.connected_count();
        check(cc == 35, "connected_count reflects real instances (1+1+1+32)");
        std::printf("  (c)/(d) healthy graph passes; connected_count=%d (expect 35)\n", cc);
        // DISABLED/HALTED do not inflate the active count
        check(reg.declared_active_count() == 4, "only the 4 SHADOW sleeves are active");
    }

    if (failures == 0) { std::printf("PASS engine_registry_test\n"); return 0; }
    std::printf("FAIL engine_registry_test (%d)\n", failures); return 1;
}
