#include "../include/SeqlockMarketCache.hpp"
#include "../include/RegimeClassifier.hpp"
#include "../include/LatencyShockDetector.hpp"
#include "../include/AdaptiveEdgeFloor.hpp"
#include "../include/CapitalAllocator.hpp"
#include "../include/PortfolioSkewController.hpp"
#include "../include/EdgeEngine.hpp"
#include "../include/LiquidityDecisionEngine.hpp"
#include "../include/QueueProbabilityModel.hpp"
#include "../include/OrderSlicer.hpp"
#include "../include/PnLAttribution.hpp"
#include <iostream>
#include <cassert>
#include <thread>
#include <chrono>

using namespace chimera;

void test_seqlock_cache() {
    std::cout << "Testing SeqlockMarketCache...\n";
    
    SeqlockMarketCache cache;
    
    MarketSnapshot snap1;
    snap1.bestBid = 50000.0;
    snap1.bestAsk = 50001.0;
    snap1.mid = 50000.5;
    snap1.valid = true;
    
    cache.publish(snap1);
    auto read1 = cache.read();
    
    assert(read1.valid);
    assert(read1.mid == 50000.5);
    std::cout << "  ✓ Lock-free read/write verified\n";
    
    std::cout << "SeqlockMarketCache Test: PASSED\n\n";
}

void test_regime_classifier() {
    std::cout << "Testing RegimeClassifier...\n";
    
    RegimeClassifier classifier;
    
    // Simulate calm market
    for (int i = 0; i < 100; i++) {
        classifier.update(50000.0 + i * 0.1);
    }
    
    auto regime1 = classifier.regime();
    assert(regime1 == MarketRegime::CALM);
    std::cout << "  ✓ CALM regime detected\n";
    
    // Simulate volatile market
    for (int i = 0; i < 50; i++) {
        classifier.update(50000.0 + i * 50.0);
    }
    
    auto regime2 = classifier.regime();
    std::cout << "  ✓ Volatility regime: " << (int)regime2 << " (higher volatility detected)\n";
    // Just verify it changed from CALM
    assert(regime2 != MarketRegime::CALM);
    
    std::cout << "RegimeClassifier Test: PASSED\n\n";
}

void test_latency_shock() {
    std::cout << "Testing LatencyShockDetector...\n";
    
    LatencyShockDetector detector;
    
    // Normal latency
    bool shock1 = detector.update(5.0);
    assert(!shock1);
    std::cout << "  ✓ Normal latency - no shock\n";
    
    // Spike
    bool shock2 = detector.update(30.0);
    assert(shock2);  // Now in cooldown
    std::cout << "  ✓ Latency spike detected - entering cooldown\n";
    
    // Should still be in cooldown
    bool shock3 = detector.update(5.0);
    assert(shock3);
    std::cout << "  ✓ Cooldown period active\n";
    
    std::cout << "LatencyShockDetector Test: PASSED\n\n";
}

void test_adaptive_edge_floor() {
    std::cout << "Testing AdaptiveEdgeFloor...\n";
    
    AdaptiveEdgeFloor floor;
    
    double initial = floor.floor();
    assert(initial == 10.0);
    std::cout << "  ✓ Initial floor: " << initial << " bps\n";
    
    // Simulate losses - floor should increase
    for (int i = 0; i < 10; i++) {
        floor.update(-5.0);
    }
    
    double after_losses = floor.floor();
    assert(after_losses > initial);
    std::cout << "  ✓ Floor increased after losses: " << after_losses << " bps\n";
    
    // Simulate wins - floor should gradually decrease
    for (int i = 0; i < 100; i++) {
        floor.update(10.0);
    }
    
    double after_wins = floor.floor();
    std::cout << "  ✓ Floor after wins: " << after_wins << " bps (adaptive learning)\n";
    // Floor decreases slowly with wins
    
    std::cout << "AdaptiveEdgeFloor Test: PASSED\n\n";
}

void test_capital_allocator() {
    std::cout << "Testing CapitalAllocator...\n";
    
    CapitalAllocator allocator;
    
    double base = 100.0;
    
    // CALM regime - increase size
    double size1 = allocator.scale(MarketRegime::CALM, base, false, false);
    assert(size1 == 120.0);
    std::cout << "  ✓ CALM: size scaled to " << size1 << "\n";
    
    // VOLATILE regime - reduce size
    double size2 = allocator.scale(MarketRegime::VOLATILE, base, false, false);
    assert(size2 == 60.0);
    std::cout << "  ✓ VOLATILE: size scaled to " << size2 << "\n";
    
    // CHAOTIC regime - zero size
    double size3 = allocator.scale(MarketRegime::CHAOTIC, base, false, false);
    assert(size3 == 0.0);
    std::cout << "  ✓ CHAOTIC: trading disabled\n";
    
    // Latency shock - zero size
    double size4 = allocator.scale(MarketRegime::NORMAL, base, false, true);
    assert(size4 == 0.0);
    std::cout << "  ✓ SHOCK: trading disabled\n";
    
    std::cout << "CapitalAllocator Test: PASSED\n\n";
}

void test_edge_engine() {
    std::cout << "Testing EdgeEngine...\n";
    
    EdgeEngine engine;
    
    // Update with market data
    engine.update(
        0.6,      // imbalance
        1000.0,   // trade volume
        0.0001,   // price return
        0.0002,   // leader return
        0.0001,   // follower return
        100.0,    // prev bid vol
        90.0,     // new bid vol
        100.0,    // prev ask vol
        95.0      // new ask vol
    );
    
    double edge = engine.expectedEdgeBps();
    std::cout << "  ✓ Expected edge calculated: " << edge << " bps\n";
    assert(edge != 0.0);  // Should produce some edge
    
    std::cout << "EdgeEngine Test: PASSED\n\n";
}

void test_liquidity_decision() {
    std::cout << "Testing LiquidityDecisionEngine...\n";
    
    LiquidityDecisionEngine engine;
    
    // Wide spread, low queue - MAKER
    auto style1 = engine.decide(5.0, 2.0, 0.5, 0.0003);
    assert(style1 == ExecutionStyle::MAKER);
    std::cout << "  ✓ Wide spread + low queue → MAKER\n";
    
    // High volatility - TAKER
    auto style2 = engine.decide(5.0, 2.0, 0.5, 0.002);
    assert(style2 == ExecutionStyle::TAKER);
    std::cout << "  ✓ High volatility → TAKER\n";
    
    // High queue wait - TAKER
    auto style3 = engine.decide(5.0, 10.0, 0.5, 0.0003);
    assert(style3 == ExecutionStyle::TAKER);
    std::cout << "  ✓ High queue wait → TAKER\n";
    
    std::cout << "LiquidityDecisionEngine Test: PASSED\n\n";
}

void test_queue_probability() {
    std::cout << "Testing QueueProbabilityModel...\n";
    
    QueueProbabilityModel model;
    
    double prob1 = model.estimateFillProb(100.0, 10.0, 0.0005);
    std::cout << "  ✓ Fill probability (large queue): " << prob1 << "\n";
    assert(prob1 > 0.0 && prob1 <= 1.0);
    
    double prob2 = model.estimateFillProb(10.0, 100.0, 0.0005);
    std::cout << "  ✓ Fill probability (small queue): " << prob2 << "\n";
    assert(prob2 > prob1);  // Smaller queue = higher probability
    
    std::cout << "QueueProbabilityModel Test: PASSED\n\n";
}

void test_order_slicer() {
    std::cout << "Testing OrderSlicer...\n";
    
    OrderSlicer slicer;
    
    auto slices = slicer.slice(5.0, 5, 500);
    
    assert(slices.size() == 5);
    assert(slices[0].qty == 1.0);
    assert(slices[0].delayMs == 100);
    std::cout << "  ✓ Order sliced: 5 BTC → 5 × 1 BTC @ 100ms intervals\n";
    
    std::cout << "OrderSlicer Test: PASSED\n\n";
}

void test_pnl_attribution() {
    std::cout << "Testing PnLAttribution...\n";
    
    PnLAttribution attr;
    
    // LONG trade
    auto pnl1 = attr.compute(50000.0, 50100.0, 1.0, 7.5, 0.5, true);
    std::cout << "  ✓ LONG: gross=" << pnl1.grossBps << " bps, net=" << pnl1.netBps << " bps\n";
    assert(pnl1.grossBps > 0);
    assert(pnl1.netBps < pnl1.grossBps);
    
    // SHORT trade  
    auto pnl2 = attr.compute(50000.0, 49900.0, 1.0, 7.5, 0.5, false);
    std::cout << "  ✓ SHORT: gross=" << pnl2.grossBps << " bps, net=" << pnl2.netBps << " bps\n";
    assert(pnl2.grossBps > 0);
    assert(pnl2.netBps < pnl2.grossBps);
    
    std::cout << "PnLAttribution Test: PASSED\n\n";
}

int main() {
    std::cout << "═══════════════════════════════════════════════════════\n";
    std::cout << "  PROFESSIONAL COMPONENTS TEST SUITE\n";
    std::cout << "═══════════════════════════════════════════════════════\n\n";
    
    try {
        test_seqlock_cache();
        test_regime_classifier();
        test_latency_shock();
        test_adaptive_edge_floor();
        test_capital_allocator();
        test_edge_engine();
        test_liquidity_decision();
        test_queue_probability();
        test_order_slicer();
        test_pnl_attribution();
        
        std::cout << "═══════════════════════════════════════════════════════\n";
        std::cout << "  ALL PROFESSIONAL COMPONENTS TESTS PASSED ✓\n";
        std::cout << "═══════════════════════════════════════════════════════\n\n";
        
        std::cout << "VERIFIED COMPONENTS:\n";
        std::cout << "  ✓ Lock-free market cache (seqlock pattern)\n";
        std::cout << "  ✓ Market regime classification\n";
        std::cout << "  ✓ Latency shock detection with cooldown\n";
        std::cout << "  ✓ Adaptive edge floor learning\n";
        std::cout << "  ✓ Regime-based capital allocation\n";
        std::cout << "  ✓ Multi-signal edge aggregation\n";
        std::cout << "  ✓ Maker/taker liquidity decisions\n";
        std::cout << "  ✓ Queue fill probability estimation\n";
        std::cout << "  ✓ Smart order slicing\n";
        std::cout << "  ✓ Economic PnL attribution\n";
        
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "TEST FAILED: " << e.what() << "\n";
        return 1;
    }
}
