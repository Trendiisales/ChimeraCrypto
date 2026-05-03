#!/usr/bin/env bash
# ============================================================================
# cleanup_dead_engines.sh
#
# Removes engine .hpp files from the ChimeraCrypto repo that are not
# referenced by main.cpp, are not transitively included by SwingEngine, and
# do not represent a strategy worth keeping for future wiring.
#
# Default mode is DRY-RUN — prints what it would do, deletes nothing.
# Pass --force to actually run `git rm` on the files.
#
# Usage:
#   ./tools/cleanup_dead_engines.sh           # dry run
#   ./tools/cleanup_dead_engines.sh --force   # actually delete (via git rm)
#
# After --force you'll have changes staged in git — review with `git status`,
# then `git commit -m "Remove dormant engine files"` if happy.
# ============================================================================

set -euo pipefail

REPO_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$REPO_ROOT"

FORCE=0
if [[ "${1:-}" == "--force" ]]; then
    FORCE=1
fi

# ----------------------------------------------------------------------------
# DISCARD list. Each entry is a path relative to the repo root.
# Categorisation comments explain why.
# ----------------------------------------------------------------------------
DISCARD_FILES=(
    # Obsolete engines explicitly replaced by SwingEngine.
    "include/core/TrendEngine.hpp"

    # Old multi-engine wrappers (the "Quad/Triple/Enhanced/Balanced" family).
    # The dashboard's "QUAD ENGINE" branding came from these. None compiled.
    "include/core/BalancedEngine.hpp"
    "include/core/EnhancedBalancedEngine.hpp"
    "include/core/TripleEngineBalancedEngine.hpp"
    "include/core/QuadEngineBalancedEngine.hpp"
    "include/core/HybridRegimeController.hpp"
    "include/core/UltraController.hpp"
    "include/core/UltraEngine.hpp"

    # Generic/structural stubs that don't carry standalone strategy.
    "include/core/AlignedEngines.hpp"
    "include/core/PositionEngine.hpp"
    "include/core/MicroSignalEngine.hpp"
    "include/core/StructuralEngine.hpp"
    "include/core/ConvexShockEngine.hpp"
    "include/core/PullbackContinuationEngine.hpp"

    # Macro lead-lag with weak edge (NGAS) + duplicate funding wrapper.
    "include/core/NGASLeadLagEngine.hpp"
    "include/core/FundingSignalEngine.hpp"

    # Top-level InstitutionalEngine (different file from include/engine/...).
    "include/InstitutionalEngine.hpp"

    # The entire include/engine/ folder is the "institutional" attempt that
    # depends on 20+ missing headers (Portfolio, PositionLedger, etc.).
    # None of it compiles or is referenced.
    "include/engine/AdaptiveFadeController.hpp"
    "include/engine/CompressionEngine.hpp"
    "include/engine/DominanceControlPlane.hpp"
    "include/engine/InstitutionalEngine.hpp"
    "include/engine/LiquidityVacuumEngine.hpp"
    "include/engine/MultiSymbolAlignmentEngine.hpp"
    "include/engine/VolatilityExpansionEngine.hpp"

    # Orphan .cpp files in src/core that aren't in CMakeLists at all.
    "src/core/Portfolio.cpp"
    "src/core/StatefulGovernor.cpp"
)

# ----------------------------------------------------------------------------
# KEEPERS — listed here for documentation only. NOT touched by this script.
# These are dormant but worth retaining because each carries a coherent,
# crypto-realistic strategy that fills a different niche. See ENGINE_ROADMAP.md
# for wiring plans.
# ----------------------------------------------------------------------------
# Keepers (15):
#   include/core/OrderbookImbalanceEngine.hpp     microstructure mean reversion
#   include/core/LiquidationEngine.hpp            short-liq -> spot long
#   include/core/FundingWindowEngine.hpp          basis snap-back at 0/8/16 UTC
#   include/core/BasisMomentumEngine.hpp          perp-leads-spot lead-lag
#   include/core/SessionMomentumEngine.hpp        London/NY/Asia open momentum
#   include/core/DivergenceEngine.hpp             cross-symbol mean reversion
#   include/core/LeadLagEngine.hpp                BTC -> alt N-symbol lead-lag
#   include/core/AggressiveFlowEngine.hpp         agg buy/sell flow imbalance
#   include/core/CompressionBreakoutEngine.hpp    vol-compression breakout
#   include/core/VolumeShockEngine.hpp            volume-confirmed momentum
#   include/core/StatArbEngine.hpp                BTC/ETH cointegration spread
#   include/core/SpreadCompressionEngine.hpp      spread-tightening directional
#   include/core/LiqBracketEngine.hpp             triple-confirmed bracket
#   include/core/CapitalScalingEngine.hpp         meta-engine for sizing
#   include/recon/ReconciliationEngine.hpp        position recon (risk layer)

# ----------------------------------------------------------------------------
# Run
# ----------------------------------------------------------------------------
echo "================================================================"
echo "  Chimera dead-engine cleanup"
echo "================================================================"
echo "  Mode:   $([[ $FORCE -eq 1 ]] && echo 'FORCE (will git rm)' || echo 'dry-run (no changes)')"
echo "  Repo:   $REPO_ROOT"
echo "  Files:  ${#DISCARD_FILES[@]} candidates"
echo "================================================================"
echo ""

missing=0
will_remove=0

for f in "${DISCARD_FILES[@]}"; do
    if [[ ! -e "$f" ]]; then
        printf "  [skip-missing]  %s\n" "$f"
        ((missing++)) || true
        continue
    fi
    if [[ $FORCE -eq 1 ]]; then
        git rm -f --quiet "$f"
        printf "  [REMOVED]       %s\n" "$f"
    else
        printf "  [would-remove]  %s\n" "$f"
    fi
    ((will_remove++)) || true
done

echo ""
echo "----------------------------------------------------------------"
echo "  Candidates:    ${#DISCARD_FILES[@]}"
echo "  Already gone:  $missing"
echo "  Acted on:      $will_remove"
if [[ $FORCE -eq 0 ]]; then
    echo ""
    echo "  Dry run — no files were modified."
    echo "  Re-run with --force to actually remove these files."
else
    echo ""
    echo "  Files removed and staged in git."
    echo "  Review with: git status"
    echo "  Commit with: git commit -m \"Remove dormant engine files\""
fi
echo "----------------------------------------------------------------"
