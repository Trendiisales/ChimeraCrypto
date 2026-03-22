#!/bin/bash
# chimera_deploy.sh — pull latest, rebuild, restart
set -e
cd ~/ChimeraCrypto

echo "[DEPLOY] Pulling latest..."
git pull origin main

echo "[DEPLOY] Building..."
cd build
make -j$(nproc) 2>&1 | tail -5

echo "[DEPLOY] Stopping old instance..."
pkill -9 chimera 2>/dev/null || true
sleep 2

echo "[DEPLOY] Starting new instance..."
cd ~/ChimeraCrypto
BUILD_HASH=$(git rev-parse --short HEAD)
echo "[DEPLOY] Running version: $BUILD_HASH"
./build/chimera &
sleep 3

# Verify correct version is running
RUNNING_HASH=$(grep "Build version" logs/chimera_$(date +%Y-%m-%d).log | tail -1 | grep -o '[a-f0-9]\{7\}')
if [ "$RUNNING_HASH" = "$BUILD_HASH" ]; then
    echo "[DEPLOY] ✓ Correct version confirmed: $BUILD_HASH"
else
    echo "[DEPLOY] ✗ VERSION MISMATCH — expected $BUILD_HASH, got $RUNNING_HASH"
    exit 1
fi
