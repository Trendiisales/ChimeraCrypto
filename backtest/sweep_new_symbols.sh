#!/bin/bash
# ============================================================================
# Session 30: Sweep optimizer_general on new symbols
# Run after download_new_symbols.py has completed.
#
# Usage: cd backtest && bash sweep_new_symbols.sh
# ============================================================================

set -e

OPTIMIZER="./optimizer_general"
COST_BP=20  # conservative for newer/less-liquid pairs

# New symbols
SYMBOLS=("pepeusdt" "wifusdt" "fetusdt" "ondousdt" "tiausdt")

# Strategies to test on each
STRATEGIES=("TSMOM" "RSI_REVERT" "BOLLINGER" "DONCHIAN")

# Timeframes to sweep
TIMEFRAMES=("H4" "H6" "H8" "H12" "D1")

RESULTS_DIR="results_session30"
mkdir -p "$RESULTS_DIR"

echo "================================================================"
echo "Session 30: New Symbol Sweep"
echo "Symbols: ${SYMBOLS[*]}"
echo "Strategies: ${STRATEGIES[*]}"
echo "Timeframes: ${TIMEFRAMES[*]}"
echo "Cost: ${COST_BP}bp"
echo "================================================================"

# Ensure optimizer is built
if [ ! -f "$OPTIMIZER" ]; then
    echo "[BUILD] Compiling optimizer_general..."
    g++ -std=c++17 -O2 -I../include optimizer_general.cpp -o optimizer_general
fi

total=0
passed=0

for sym in "${SYMBOLS[@]}"; do
    for strat in "${STRATEGIES[@]}"; do
        for tf in "${TIMEFRAMES[@]}"; do
            total=$((total + 1))
            outfile="$RESULTS_DIR/${sym}_${strat}_${tf}.txt"
            echo ""
            echo "────────────────────────────────────────────────────────"
            echo "[SWEEP] $sym $strat $tf (cost=${COST_BP}bp)"
            echo "────────────────────────────────────────────────────────"

            $OPTIMIZER "$sym" "$strat" "$tf" "$COST_BP" > "$outfile" 2>&1 || true

            # Check if any result passed deploy criteria
            # PF >= 1.3, Nbr >= 60%, OOS trades >= 20
            if grep -q "DEPLOY_GRADE" "$outfile" 2>/dev/null; then
                passed=$((passed + 1))
                echo "  ✓ DEPLOY-GRADE result found!"
                grep "DEPLOY_GRADE" "$outfile" | head -3
            else
                echo "  ✗ No deploy-grade combos"
            fi
        done
    done
done

echo ""
echo "================================================================"
echo "SWEEP COMPLETE: $total combos tested, $passed deploy-grade"
echo "Results in: $RESULTS_DIR/"
echo ""
echo "Next: run walk_forward_validate on each DEPLOY_GRADE result"
echo "================================================================"
