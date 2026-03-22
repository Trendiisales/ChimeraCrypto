#!/bin/bash
# chimera_deploy.sh
cd ~/ChimeraCrypto

echo "[DEPLOY] Pulling latest..."
git pull origin main

echo "[DEPLOY] Building..."
cd ~/ChimeraCrypto/build
if ! make -j$(nproc); then
    echo "[DEPLOY] BUILD FAILED"
    exit 1
fi
cd ~/ChimeraCrypto

BUILD_HASH=$(git rev-parse --short HEAD)
echo "[DEPLOY] Built version: $BUILD_HASH"

echo "[DEPLOY] Stopping old instance..."
pkill -9 chimera 2>/dev/null || true
sleep 3

echo "[DEPLOY] Starting chimera..."
LOG=~/ChimeraCrypto/logs/chimera_$(date +%Y-%m-%d).log
mkdir -p ~/ChimeraCrypto/logs
nohup ~/ChimeraCrypto/build/chimera >> $LOG 2>&1 &
CHIMERA_PID=$!
sleep 4

if kill -0 $CHIMERA_PID 2>/dev/null; then
    echo "[DEPLOY] chimera running PID=$CHIMERA_PID"
    RUNNING_HASH=$(grep "Build version" $LOG | tail -1 | grep -oE "[a-f0-9]{7}" | head -1)
    if [ "$RUNNING_HASH" = "$BUILD_HASH" ]; then
        echo "[DEPLOY] VERSION OK: $RUNNING_HASH"
    else
        echo "[DEPLOY] WARNING: expected $BUILD_HASH got $RUNNING_HASH"
    fi
else
    echo "[DEPLOY] CHIMERA CRASHED — last log lines:"
    tail -20 $LOG
    exit 1
fi
