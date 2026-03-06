#!/bin/bash
# ============================================================
# Binance VPS Latency Diagnostic
# Tests: ICMP ping, TCP connect, REST API round-trip, WS feed
# Run on VPS: bash tools/ping_binance.sh
# ============================================================

echo "╔════════════════════════════════════════════════════════╗"
echo "║         CHIMERA - BINANCE LATENCY DIAGNOSTIC          ║"
echo "╚════════════════════════════════════════════════════════╝"
echo ""
echo "VPS: $(hostname) | $(curl -s ifconfig.me 2>/dev/null || echo 'IP unknown')"
echo "Date: $(date -u '+%Y-%m-%d %H:%M:%S UTC')"
echo ""

# ── 1. ICMP PING ─────────────────────────────────────────────
echo "── 1. ICMP PING (stream.binance.com) ──────────────────────"
ping -c 10 -i 0.2 stream.binance.com 2>/dev/null | tail -2
echo ""

echo "── 1b. ICMP PING (api.binance.com) ────────────────────────"
ping -c 10 -i 0.2 api.binance.com 2>/dev/null | tail -2
echo ""

# ── 2. TCP CONNECT TIME ──────────────────────────────────────
echo "── 2. TCP CONNECT TIME ─────────────────────────────────────"
for i in 1 2 3 4 5; do
    result=$(curl -o /dev/null -s -w "%{time_connect}" \
        --connect-timeout 5 \
        "https://api.binance.com/api/v3/ping" 2>/dev/null)
    echo "  TCP connect attempt $i: ${result}s"
done
echo ""

# ── 3. REST API ROUND-TRIP ────────────────────────────────────
echo "── 3. REST API ROUND-TRIP (api.binance.com/api/v3/time) ───"
for i in 1 2 3 4 5 6 7 8 9 10; do
    local_before=$(date +%s%3N)
    server_time=$(curl -s --connect-timeout 5 \
        "https://api.binance.com/api/v3/time" 2>/dev/null | \
        grep -o '"serverTime":[0-9]*' | grep -o '[0-9]*$')
    local_after=$(date +%s%3N)
    if [ -n "$server_time" ]; then
        rtt=$((local_after - local_before))
        # One-way estimate = RTT/2, clock offset = serverTime - (before + RTT/2)
        mid=$((local_before + rtt/2))
        offset=$((server_time - mid))
        echo "  Sample $i: RTT=${rtt}ms | clock_offset=${offset}ms"
    else
        echo "  Sample $i: FAILED"
    fi
done
echo ""

# ── 4. ORDERBOOK SNAPSHOT LATENCY ────────────────────────────
echo "── 4. ORDERBOOK SNAPSHOT (api.binance.com/api/v3/depth) ───"
for sym in BTCUSDT ETHUSDT SOLUSDT; do
    t1=$(date +%s%3N)
    result=$(curl -s --connect-timeout 5 \
        "https://api.binance.com/api/v3/depth?symbol=${sym}&limit=5" 2>/dev/null)
    t2=$(date +%s%3N)
    rtt=$((t2 - t1))
    bid=$(echo "$result" | grep -o '"bids":\[\["[0-9.]*' | grep -o '[0-9.]*$' | head -1)
    ask=$(echo "$result" | grep -o '"asks":\[\["[0-9.]*' | grep -o '[0-9.]*$' | head -1)
    echo "  $sym: RTT=${rtt}ms | best_bid=${bid} | best_ask=${ask}"
done
echo ""

# ── 5. WEBSOCKET CONNECT TIME ────────────────────────────────
echo "── 5. WEBSOCKET CONNECT + FIRST MESSAGE TIME ──────────────"
if command -v wscat &>/dev/null; then
    t1=$(date +%s%3N)
    msg=$(timeout 5 wscat -c "wss://stream.binance.com:9443/stream?streams=btcusdt@bookTicker" \
        2>/dev/null | head -1)
    t2=$(date +%s%3N)
    rtt=$((t2 - t1))
    echo "  WS connect + first bookTicker: ${rtt}ms"
    echo "  Message: ${msg:0:120}..."
elif command -v websocat &>/dev/null; then
    t1=$(date +%s%3N)
    msg=$(timeout 5 websocat "wss://stream.binance.com:9443/stream?streams=btcusdt@bookTicker" \
        2>/dev/null | head -1)
    t2=$(date +%s%3N)
    rtt=$((t2 - t1))
    echo "  WS connect + first bookTicker: ${rtt}ms"
    echo "  Message: ${msg:0:120}..."
else
    # Fallback: use curl to test WSS endpoint reachability
    echo "  wscat/websocat not installed - testing TCP to WS port..."
    t1=$(date +%s%3N)
    curl -o /dev/null -s -w "  TCP to stream.binance.com:9443 = %{time_connect}s\n" \
        --connect-timeout 5 \
        "https://stream.binance.com:9443" 2>/dev/null || \
    echo "  TCP connect time to :9443:"
    nc -zw3 stream.binance.com 9443 2>/dev/null && echo "  Port 9443: OPEN" || echo "  Port 9443: BLOCKED"
    t2=$(date +%s%3N)
fi
echo ""

# ── 6. TRACEROUTE HOPS ───────────────────────────────────────
echo "── 6. ROUTE TO BINANCE (first 8 hops) ─────────────────────"
traceroute -m 8 -w 2 stream.binance.com 2>/dev/null || \
tracepath -m 8 stream.binance.com 2>/dev/null || \
echo "  traceroute not available"
echo ""

# ── 7. SUMMARY ───────────────────────────────────────────────
echo "── 7. SYSTEM ───────────────────────────────────────────────"
echo "  CPU cores: $(nproc)"
echo "  RAM: $(free -h | awk '/^Mem:/{print $2}') total, $(free -h | awk '/^Mem:/{print $7}') available"
echo "  Kernel: $(uname -r)"
echo "  Region hint: $(curl -s --connect-timeout 3 https://ipinfo.io/org 2>/dev/null || echo 'unknown')"
echo ""
echo "Done."
