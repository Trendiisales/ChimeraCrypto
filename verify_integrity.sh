#!/bin/bash
# ChimeraCrypto Integrity Verification Script
# Run this after extracting the tarball to verify all fixes are in place

echo "================================================"
echo "ChimeraCrypto Fixed Version Integrity Check"
echo "================================================"
echo ""

ERRORS=0

# Check 1: main.cpp includes WsTelemetryServer
echo -n "[1/7] Checking main.cpp includes WsTelemetryServer.hpp... "
if grep -q '#include "telemetry/WsTelemetryServer.hpp"' src/main.cpp; then
    echo "✓ PASS"
else
    echo "✗ FAIL"
    ERRORS=$((ERRORS + 1))
fi

# Check 2: main.cpp uses WsTelemetryServer
echo -n "[2/7] Checking main.cpp instantiates WsTelemetryServer... "
if grep -q "WsTelemetryServer server(9001" src/main.cpp; then
    echo "✓ PASS"
else
    echo "✗ FAIL"
    ERRORS=$((ERRORS + 1))
fi

# Check 3: main.cpp does NOT use HttpTelemetryServer forward declaration
echo -n "[3/7] Checking main.cpp removed HttpTelemetryServer... "
if ! grep -q "class HttpTelemetryServer" src/main.cpp; then
    echo "✓ PASS"
else
    echo "✗ FAIL (HttpTelemetryServer forward declaration still present)"
    ERRORS=$((ERRORS + 1))
fi

# Check 4: app.js connects to /ws endpoint
echo -n "[4/7] Checking app.js connects to /ws endpoint... "
if grep -q 'WebSocket.*location.host.*/ws' gui/app.js; then
    echo "✓ PASS"
else
    echo "✗ FAIL"
    ERRORS=$((ERRORS + 1))
fi

# Check 5: nginx config exists
echo -n "[5/7] Checking nginx configuration exists... "
if [ -f config/nginx_chimera.conf ]; then
    echo "✓ PASS"
else
    echo "✗ FAIL"
    ERRORS=$((ERRORS + 1))
fi

# Check 6: WsTelemetryServer header exists
echo -n "[6/7] Checking WsTelemetryServer header exists... "
if [ -f include/telemetry/WsTelemetryServer.hpp ]; then
    echo "✓ PASS"
else
    echo "✗ FAIL"
    ERRORS=$((ERRORS + 1))
fi

# Check 7: WsTelemetryServer implementation exists
echo -n "[7/7] Checking WsTelemetryServer implementation exists... "
if [ -f src/telemetry/WsTelemetryServer.cpp ]; then
    echo "✓ PASS"
else
    echo "✗ FAIL"
    ERRORS=$((ERRORS + 1))
fi

echo ""
echo "================================================"
if [ $ERRORS -eq 0 ]; then
    echo "✓ ALL CHECKS PASSED"
    echo "This tarball is verified as the FIXED version."
    echo ""
    echo "Next steps:"
    echo "  1. Read DEPLOYMENT.md for installation instructions"
    echo "  2. Read FORENSIC_AUDIT.md to understand what was fixed"
    echo "  3. Build: mkdir build && cd build && cmake .. && make"
    echo "  4. Configure nginx using config/nginx_chimera.conf"
    echo "  5. Run: ./chimera"
    exit 0
else
    echo "✗ $ERRORS CHECKS FAILED"
    echo "This tarball may be corrupted or not the fixed version."
    echo "Please re-extract from ChimeraCrypto_FIXED.tar.gz"
    exit 1
fi
