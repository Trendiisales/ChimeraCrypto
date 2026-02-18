#!/bin/bash
set -e

echo "═══════════════════════════════════════════════════"
echo "  CHIMERA CRYPTO - BUILD & VERIFICATION"
echo "═══════════════════════════════════════════════════"
echo ""

# Check dependencies
echo "[1/6] Checking dependencies..."
missing=()

if ! command -v cmake &> /dev/null; then
    missing+=("cmake")
fi

if ! command -v g++ &> /dev/null; then
    missing+=("g++")
fi

if ! pkg-config --exists openssl; then
    missing+=("libssl-dev")
fi

if ! pkg-config --exists libcurl; then
    missing+=("libcurl4-openssl-dev")
fi

if [ ! -f /usr/include/libwebsockets.h ] && [ ! -f /usr/local/include/libwebsockets.h ]; then
    missing+=("libwebsockets-dev")
fi

if [ ${#missing[@]} -gt 0 ]; then
    echo "❌ Missing dependencies: ${missing[*]}"
    echo ""
    echo "Install with:"
    echo "sudo apt-get install -y cmake g++ libssl-dev libcurl4-openssl-dev libwebsockets-dev zlib1g-dev"
    exit 1
fi

echo "✅ All dependencies found"
echo ""

# Clean build
echo "[2/6] Cleaning build directory..."
rm -rf build
mkdir build
cd build
echo "✅ Build directory ready"
echo ""

# CMake
echo "[3/6] Running CMake..."
if cmake -DCMAKE_BUILD_TYPE=Release .. > cmake.log 2>&1; then
    echo "✅ CMake configuration successful"
else
    echo "❌ CMake failed. Check build/cmake.log"
    tail -20 cmake.log
    exit 1
fi
echo ""

# Compile
echo "[4/6] Compiling (this may take a minute)..."
if make -j$(nproc) > make.log 2>&1; then
    echo "✅ Compilation successful"
else
    echo "❌ Compilation failed. Check build/make.log"
    tail -30 make.log
    exit 1
fi
echo ""

# Verify binary
echo "[5/6] Verifying binary..."
if [ -f chimera ]; then
    SIZE=$(stat -f%z chimera 2>/dev/null || stat -c%s chimera)
    echo "✅ Binary created: chimera (${SIZE} bytes)"
    
    # Check for SIMD
    if strings chimera | grep -q "AVX2"; then
        echo "✅ AVX2 optimization detected"
    fi
else
    echo "❌ Binary not found"
    exit 1
fi
echo ""

# File structure check
echo "[6/6] Verifying file structure..."
cd ..
FILES=(
    "include/l2/L2Types.hpp"
    "include/l2/L2Book.hpp"
    "include/l2/L2Bootstrapper.hpp"
    "include/l2/SnapshotFetcher.hpp"
    "include/DepthManager.hpp"
    "src/l2/L2Book.cpp"
    "src/l2/L2Bootstrapper.cpp"
    "src/l2/SnapshotFetcher.cpp"
    "src/DepthManager.cpp"
    "dashboard_standalone.html"
    "README.md"
)

all_present=true
for file in "${FILES[@]}"; do
    if [ ! -f "$file" ]; then
        echo "❌ Missing: $file"
        all_present=false
    fi
done

if $all_present; then
    echo "✅ All L2 bootstrap files present"
else
    exit 1
fi
echo ""

echo "═══════════════════════════════════════════════════"
echo "  ✅ BUILD SUCCESSFUL"
echo "═══════════════════════════════════════════════════"
echo ""
echo "Run engine:"
echo "  cd build && ./chimera"
echo ""
echo "Open dashboard:"
echo "  http://localhost:8888"
echo "  or open dashboard_standalone.html in browser"
echo ""
echo "Test REST snapshot:"
echo "  curl 'https://api.binance.com/api/v3/depth?symbol=BTCUSDT&limit=10'"
echo ""
