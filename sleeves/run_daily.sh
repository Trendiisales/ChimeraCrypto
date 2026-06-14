#!/bin/bash
# Chimera shadow sleeves — daily run. Update prices, validate, log target portfolio.
# SHADOW ONLY: writes target weights to the ledger, places NO orders.
set -e
cd "$(dirname "$0")"
echo "===== $(date -u +%Y-%m-%dT%H:%MZ) sleeves daily run ====="
python3 update_data.py 2>&1 | tail -5
# hard data-veracity gate — refuse to compute on bad/stale/mixed data
if ! python3 validate_dataset.py data/multiyr --require-cycles 2022,2023,2024 >/tmp/sleeve_validate.log 2>&1; then
    echo "DATA VALIDATION FAILED — skipping sleeve compute. See /tmp/sleeve_validate.log"
    tail -6 /tmp/sleeve_validate.log
    exit 1
fi
tail -1 /tmp/sleeve_validate.log
python3 chimera_sleeves.py shadow 2>&1
python3 gui_data.py 2>&1 | tail -1
echo "===== done ====="
