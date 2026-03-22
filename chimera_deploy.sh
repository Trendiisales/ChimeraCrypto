#!/bin/bash
cd ~/ChimeraCrypto
git pull origin main
cd build && make -j$(nproc) || { echo "BUILD FAILED"; exit 1; }
cd ~/ChimeraCrypto
pkill -9 chimera 2>/dev/null || true
sleep 1
./build/chimera
