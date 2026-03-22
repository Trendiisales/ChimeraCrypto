#!/bin/bash
# chimera_deploy.sh — pull, build, restart, verify
cd ~/ChimeraCrypto

echo "[DEPLOY] Pulling latest..."
git pull origin main

echo "[DEPLOY] Building..."
cd build
if ! make -j$(nproc) 2>&1; then
    echo "[DEPLOY] BUILD FAILED — aborting, old binary still running"
    exit 1
fi
cd ~/ChimeraCrypto

BUILD_HASH=$(git rev-parse --short HEAD)
echo "[DEPLOY] Built version: $BUILD_HASH"

echo "[DEPLOY] Stopping old instance..."
pkill -9 chimera 2>/dev/null || true
sleep 2

echo "[DEPLOY] Starting chimera..."
nohup ./build/chimera >> logs/chimera_$(date +%Y-%m-%d).log 2>&1 &
CHIMERA_PID=$!
sleep 3

# Confirm it is still running
if kill -0 $CHIMERA_PID 2>/dev/null; then
    echo "[DEPLOY] ✓ chimera running (PID $CHIMERA_PID)"
    echo $CHIMERA_PID > /tmp/chimera.pid
else
    echo "[DEPLOY] ✗ chimera crashed on startup — check logs"
    tail -20 logs/chimera_$(date +%Y-%m-%d).log
    exit 1
fi

# Confirm version matches
sleep 2
RUNNING_HASH=$(grep "Build version" logs/chimera_$(date +%Y-%m-%d).log | tail -1 | grep -oE '[a-f0-9]{7}')
if [ "$RUNNING_HASH" = "$BUILD_HASH" ]; then
    echo "[DEPLOY] ✓ Version confirmed: $RUNNING_HASH"
else
    echo "[DEPLOY] ✗ Version mismatch — expected $BUILD_HASH got $RUNNING_HASH"
fi
