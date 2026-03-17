#!/bin/bash
# =============================================================================
# Chimera Deploy Script
# Run on VPS: bash scripts/deploy.sh
# Pulls latest code, rebuilds, restarts service, updates nginx.
# =============================================================================
set -e

CHIMERA_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
echo "[DEPLOY] Chimera root: $CHIMERA_DIR"
echo "[DEPLOY] User home:    $HOME"

# ── 1. Pull latest code ──────────────────────────────────────────────────────
echo "[DEPLOY] Pulling latest code..."
cd "$CHIMERA_DIR"
git pull --rebase

# ── 2. Build ─────────────────────────────────────────────────────────────────
echo "[DEPLOY] Building..."
mkdir -p build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS="-O3 -march=native" 2>&1
make -j$(nproc) 2>&1
echo "[DEPLOY] Build complete."
cd "$CHIMERA_DIR"

# ── 3. Stop running instance ──────────────────────────────────────────────────
echo "[DEPLOY] Stopping existing Chimera instance..."
if systemctl is-active --quiet chimera 2>/dev/null; then
    sudo systemctl stop chimera
    echo "[DEPLOY] Service stopped."
elif [ -f /tmp/chimera.lock ]; then
    EXISTING_PID=$(cat /tmp/chimera.lock 2>/dev/null | tr -d '\n')
    if [ -n "$EXISTING_PID" ] && kill -0 "$EXISTING_PID" 2>/dev/null; then
        kill "$EXISTING_PID"
        sleep 1
        echo "[DEPLOY] Killed PID $EXISTING_PID"
    fi
fi

# ── 4. Update nginx config ────────────────────────────────────────────────────
echo "[DEPLOY] Updating nginx config..."
NGINX_CONF="/etc/nginx/sites-available/chimera"
cp "$CHIMERA_DIR/config/nginx_chimera.conf" /tmp/chimera_nginx.conf
sed -i "s|CHIMERA_GUI_ROOT|$CHIMERA_DIR/gui|g" /tmp/chimera_nginx.conf

if ! diff -q /tmp/chimera_nginx.conf "$NGINX_CONF" > /dev/null 2>&1; then
    sudo cp /tmp/chimera_nginx.conf "$NGINX_CONF"
    if [ ! -L /etc/nginx/sites-enabled/chimera ]; then
        sudo ln -sf "$NGINX_CONF" /etc/nginx/sites-enabled/chimera
    fi
    sudo nginx -t && sudo systemctl reload nginx
    echo "[DEPLOY] nginx config updated and reloaded."
else
    echo "[DEPLOY] nginx config unchanged."
fi

# ── 5. Start Chimera ──────────────────────────────────────────────────────────
echo "[DEPLOY] Starting Chimera..."
if systemctl list-unit-files chimera.service > /dev/null 2>&1; then
    # Update service file ExecStart path in case it moved
    sudo sed -i "s|ExecStart=.*|ExecStart=$CHIMERA_DIR/build/chimera|g" /etc/systemd/system/chimera.service
    sudo sed -i "s|WorkingDirectory=.*|WorkingDirectory=$CHIMERA_DIR|g" /etc/systemd/system/chimera.service
    sudo systemctl daemon-reload
    sudo systemctl start chimera
    sleep 2
    if systemctl is-active --quiet chimera; then
        echo "[DEPLOY] Service started successfully."
    else
        echo "[DEPLOY] WARNING: Service failed to start. Check: journalctl -u chimera -n 50"
        exit 1
    fi
else
    echo "[DEPLOY] ERROR: chimera.service not installed. Install it first:"
    echo "  sudo cp $CHIMERA_DIR/scripts/chimera.service /etc/systemd/system/"
    echo "  sudo systemctl daemon-reload"
    echo "  sudo systemctl start chimera"
    echo ""
    echo "  Do NOT use 'systemctl enable chimera' — that causes auto-start on boot."
    exit 1
fi

# ── 6. Verify ────────────────────────────────────────────────────────────────
echo "[DEPLOY] Verifying /api/state endpoint..."
sleep 1
HTTP_CODE=$(curl -sk -o /dev/null -w "%{http_code}" "http://127.0.0.1:8080/api/state" 2>/dev/null || echo "000")
if [ "$HTTP_CODE" = "200" ]; then
    echo "[DEPLOY] /api/state -> HTTP 200 OK. GUI telemetry is live."
else
    echo "[DEPLOY] WARNING: /api/state returned HTTP $HTTP_CODE. Backend may still be warming up."
    echo "[DEPLOY] Retry manually: curl http://127.0.0.1:8080/api/state"
fi

echo ""
echo "=========================================="
echo " DEPLOY COMPLETE"
echo " GUI:    https://$(hostname -I | awk '{print $1}'):9443"
echo " API:    http://127.0.0.1:8080/api/state"
echo " Logs:   tail -f $CHIMERA_DIR/logs/chimera_$(date +%Y-%m-%d).log"
echo " Status: systemctl status chimera"
echo "=========================================="
