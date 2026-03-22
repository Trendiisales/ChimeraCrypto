#!/bin/bash
cd ~/ChimeraCrypto
git pull origin main
cd build && make -j$(nproc) || { echo "BUILD FAILED"; exit 1; }
cd ~/ChimeraCrypto
mkdir -p logs
pkill -9 chimera 2>/dev/null || true
sleep 1
nohup ./build/chimera > logs/chimera_$(date +%Y-%m-%d).log 2>&1 &
echo "Started: $(git rev-parse --short HEAD)"
