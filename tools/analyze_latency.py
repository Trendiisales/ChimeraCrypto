#!/usr/bin/env python3
"""
Latency Analysis Tool for ChimeraCrypto

Analyzes latency patterns and identifies bottlenecks across symbols.
Reads from telemetry JSON output or log files.
"""

import json
import sys
from typing import Dict, List, Tuple
from dataclasses import dataclass

@dataclass
class LatencyStats:
    symbol: str
    ticks: int
    evals: int
    blocks: int
    block_rate: float
    eval_rate: float
    mean_latency: float
    p95_latency: float
    histogram: Dict[str, int]

def analyze_latency_disparity(stats: List[LatencyStats]) -> None:
    """Identify symbols with disproportionate latency blocking."""
    
    print("\n" + "=" * 80)
    print("LATENCY DISPARITY ANALYSIS")
    print("=" * 80)
    
    # Calculate relative load
    total_ticks = sum(s.ticks for s in stats)
    total_evals = sum(s.evals for s in stats)
    total_blocks = sum(s.blocks for s in stats)
    
    print(f"\nGlobal Stats:")
    print(f"  Total Ticks:  {total_ticks:,}")
    print(f"  Total Evals:  {total_evals:,}")
    print(f"  Total Blocks: {total_blocks:,}")
    print(f"  Global Block Rate: {(total_blocks / total_evals * 100) if total_evals > 0 else 0:.1f}%")
    
    # Per-symbol breakdown
    print(f"\nPer-Symbol Breakdown:")
    print(f"{'Symbol':<12} {'Tick%':<8} {'Eval%':<8} {'Block%':<8} {'BlockRate':<10} {'Diagnosis'}")
    print("-" * 80)
    
    for s in stats:
        tick_pct = (s.ticks / total_ticks * 100) if total_ticks > 0 else 0
        eval_pct = (s.evals / total_evals * 100) if total_evals > 0 else 0
        block_pct = (s.blocks / total_blocks * 100) if total_blocks > 0 else 0
        
        # Diagnosis
        diagnosis = []
        
        # Check for disproportionate blocking
        if block_pct > eval_pct * 1.5:
            diagnosis.append("HIGH_BLOCK")
        
        # Check for evaluation frequency
        if s.eval_rate > 0.5:
            diagnosis.append("HIGH_EVAL_FREQ")
        
        # Check for latency issues
        if s.mean_latency > 15.0:
            diagnosis.append("HIGH_LATENCY")
        
        diagnosis_str = " | ".join(diagnosis) if diagnosis else "OK"
        
        print(f"{s.symbol:<12} {tick_pct:>6.1f}% {eval_pct:>6.1f}% {block_pct:>6.1f}% {s.block_rate*100:>8.1f}% {diagnosis_str}")
    
    # Root cause analysis
    print(f"\n{'='*80}")
    print("ROOT CAUSE ANALYSIS")
    print("=" * 80)
    
    # Find symbol with highest block rate
    max_block_symbol = max(stats, key=lambda s: s.block_rate)
    
    if max_block_symbol.block_rate > 0.15:
        print(f"\n⚠ {max_block_symbol.symbol} has disproportionate blocking ({max_block_symbol.block_rate*100:.1f}%)")
        print(f"\nPossible Causes:")
        
        # Check if it's due to higher evaluation frequency
        avg_eval_rate = sum(s.eval_rate for s in stats) / len(stats)
        if max_block_symbol.eval_rate > avg_eval_rate * 1.3:
            print(f"  ✓ Higher evaluation frequency ({max_block_symbol.eval_rate:.2f} vs avg {avg_eval_rate:.2f})")
            print(f"    → More tick processing per update")
            print(f"    → Consider reducing evaluation frequency for this symbol")
        
        # Check if it's due to higher latency
        avg_latency = sum(s.mean_latency for s in stats) / len(stats)
        if max_block_symbol.mean_latency > avg_latency * 1.2:
            print(f"  ✓ Higher mean latency ({max_block_symbol.mean_latency:.1f}ms vs avg {avg_latency:.1f}ms)")
            print(f"    → Network path issue or orderbook depth")
            print(f"    → Check p95 latency: {max_block_symbol.p95_latency:.1f}ms")
        
        # Check histogram for tail latency
        if max_block_symbol.histogram:
            total_samples = sum(max_block_symbol.histogram.values())
            slow_samples = max_block_symbol.histogram.get('25_50ms', 0) + \
                          max_block_symbol.histogram.get('50ms_plus', 0)
            slow_pct = (slow_samples / total_samples * 100) if total_samples > 0 else 0
            
            if slow_pct > 10:
                print(f"  ✓ High tail latency ({slow_pct:.1f}% samples >25ms)")
                print(f"    → Frequent latency spikes")
                print(f"    → Consider CPU affinity or network optimization")

def analyze_band_performance(data: Dict) -> None:
    """Analyze PnL by latency band to inform policy tuning."""
    
    if 'capital_allocation' not in data or 'symbols' not in data['capital_allocation']:
        print("\nNo band performance data available")
        return
    
    print("\n" + "=" * 80)
    print("PnL BY LATENCY BAND")
    print("=" * 80)
    
    symbols_data = data['capital_allocation']['symbols']
    
    print(f"\n{'Symbol':<12} {'Band':<12} {'Trades':<8} {'AvgPnL':<10} {'AvgSlip':<10} {'Status'}")
    print("-" * 80)
    
    for symbol, stats in symbols_data.items():
        if 'band_performance' not in stats:
            continue
        
        bands = stats['band_performance']
        
        for band_name, band_data in [('FAST', 'fast'), ('MEDIUM', 'medium'), ('SLOW', 'slow')]:
            if band_data not in bands:
                continue
            
            band = bands[band_data]
            trades = band['trades']
            avg_pnl = band['avg_pnl_bps']
            avg_slip = band['avg_slippage_bps']
            profitable = band['profitable']
            
            status = "✓ PROFIT" if profitable else "✗ LOSS"
            
            print(f"{symbol:<12} {band_name:<12} {trades:<8} {avg_pnl:>8.1f}bp {avg_slip:>8.1f}bp {status}")
    
    # Recommendations
    print(f"\n{'='*80}")
    print("RECOMMENDATIONS")
    print("=" * 80)
    
    for symbol, stats in symbols_data.items():
        if 'band_performance' not in stats:
            continue
        
        bands = stats['band_performance']
        
        # Check if SLOW band is unprofitable
        if 'slow' in bands and bands['slow']['trades'] >= 10:
            if not bands['slow']['profitable']:
                print(f"\n⚠ {symbol} SLOW band (25-50ms) is unprofitable")
                print(f"  → Recommendation: Tighten latency limit to 25ms")
                print(f"  → Expected impact: Block {bands['slow']['trades']} trades, "
                      f"avoid {abs(bands['slow']['avg_pnl_bps']):.1f}bp losses")
        
        # Check if MEDIUM band is profitable
        if 'medium' in bands and bands['medium']['trades'] >= 10:
            if bands['medium']['profitable'] and bands['medium']['avg_pnl_bps'] > 5.0:
                print(f"\n✓ {symbol} MEDIUM band (15-25ms) is profitable ({bands['medium']['avg_pnl_bps']:.1f}bp)")
                print(f"  → Recommendation: Could relax limit to 30ms if needed")
                print(f"  → Current performance suggests headroom for less aggressive filtering")

def main():
    if len(sys.argv) < 2:
        print("Usage: python analyze_latency.py <telemetry.json>")
        sys.exit(1)
    
    with open(sys.argv[1], 'r') as f:
        data = json.load(f)
    
    # Parse latency diagnostics
    if 'latency_diagnostics' in data:
        diag = data['latency_diagnostics']
        stats = []
        
        for symbol, sym_data in diag.items():
            hist = sym_data.get('histogram', {})
            stats.append(LatencyStats(
                symbol=symbol,
                ticks=sym_data['ticks'],
                evals=sym_data['evals'],
                blocks=sym_data['blocks'],
                block_rate=sym_data['block_rate'],
                eval_rate=sym_data['eval_rate'],
                mean_latency=sym_data['mean_latency_ms'],
                p95_latency=sym_data.get('p95_latency_ms', 0.0),
                histogram=hist
            ))
        
        analyze_latency_disparity(stats)
    
    # Analyze band performance
    analyze_band_performance(data)

if __name__ == '__main__':
    main()
