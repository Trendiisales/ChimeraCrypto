# ChimeraCrypto Deployment Guide

## Architecture Overview

```
┌─────────────────────────────────────────────────────────────┐
│                         Browser                              │
│                  https://hostname:9443                       │
└──────────────────────────┬──────────────────────────────────┘
                           │
                           ▼
┌─────────────────────────────────────────────────────────────┐
│                      nginx (Port 9443)                       │
│                                                              │
│  ┌──────────────┐              ┌─────────────────────┐     │
│  │ Static Files │              │   WebSocket Proxy   │     │
│  │   /gui/      │              │      /ws → :9001    │     │
│  └──────────────┘              └─────────────────────┘     │
└──────────────────────────┬──────────────────────────────────┘
                           │
                           ▼
┌─────────────────────────────────────────────────────────────┐
│                Chimera Backend (Port 9001)                   │
│                      localhost only                          │
│                                                              │
│  ┌────────────────────────────────────────────────┐         │
│  │         WsTelemetryServer                      │         │
│  │    (libwebsockets, RFC6455 compliant)         │         │
│  └────────────────────────────────────────────────┘         │
│                           │                                  │
│                           ▼                                  │
│  ┌────────────────────────────────────────────────┐         │
│  │            TelemetrySpine                      │         │
│  │        (Real-time market data)                 │         │
│  └────────────────────────────────────────────────┘         │
└─────────────────────────────────────────────────────────────┘
```

## What Was Fixed

### 1. main.cpp
- **Before**: Used HttpTelemetryServer with forward declaration (broken)
- **After**: Uses WsTelemetryServer with proper header include
- **Result**: Clean WebSocket implementation, no ABI issues

### 2. gui/app.js
- **Before**: Connected to root path `ws://host/`
- **After**: Connects to `/ws` endpoint via nginx
- **Added**: Automatic reconnection on disconnect
- **Added**: Error handling and logging

### 3. Architecture
- **nginx**: Serves static files, proxies WebSocket
- **Chimera**: Only binds localhost:9001 (not exposed)
- **Transport**: WebSocket for real-time telemetry
- **Security**: SSL/TLS handled by nginx

## Installation Steps

### Step 1: Build Chimera

```bash
cd /path/to/ChimeraCrypto
mkdir build
cd build
cmake ..
make -j$(nproc)
```

**Expected output:**
```
[ 98%] Building CXX object CMakeFiles/chimera.dir/src/telemetry/WsTelemetryServer.cpp.o
[100%] Linking CXX executable chimera
```

**Build requirements:**
- libwebsockets-dev
- libssl-dev
- libcurl4-openssl-dev
- cmake >= 3.16
- g++ with C++20 support

### Step 2: Install nginx

```bash
sudo apt update
sudo apt install nginx
```

### Step 3: Generate SSL Certificate

**For testing (self-signed):**
```bash
sudo mkdir -p /etc/ssl/private
sudo openssl req -x509 -nodes -days 365 -newkey rsa:2048 \
    -keyout /etc/ssl/private/chimera.key \
    -out /etc/ssl/certs/chimera.crt \
    -subj "/C=US/ST=State/L=City/O=Chimera/CN=localhost"
```

**For production:**
Use Let's Encrypt or your organization's certificate.

### Step 4: Configure nginx

```bash
# Copy nginx config
sudo cp config/nginx_chimera.conf /etc/nginx/sites-available/chimera

# Update paths in the config file
sudo nano /etc/nginx/sites-available/chimera
# Change: root /home/jo/ChimeraCrypto/gui;
# To:     root /your/actual/path/to/ChimeraCrypto/gui;

# Enable the site
sudo ln -s /etc/nginx/sites-available/chimera /etc/nginx/sites-enabled/

# Test configuration
sudo nginx -t

# If test passes, reload nginx
sudo systemctl reload nginx
```

### Step 5: Run Chimera

```bash
cd /path/to/ChimeraCrypto/build
./chimera
```

**Expected output:**
```
ChimeraCrypto engine running
WebSocket telemetry on 127.0.0.1:9001
Configure nginx to:
  - Serve GUI from /gui directory
  - Proxy /ws to http://127.0.0.1:9001
WS telemetry listening on port 9001
```

### Step 6: Access GUI

Open browser to: `https://localhost:9443`

(Accept self-signed certificate warning if using test cert)

You should see:
- Real-time updates to all telemetry fields
- WebSocket connection status in browser console
- No 502 errors
- No Content-Length mismatches
- No "Unknown client spec version 0" errors

## Troubleshooting

### Issue: nginx fails to start
**Check:**
```bash
sudo nginx -t
sudo journalctl -u nginx -n 50
```

**Common causes:**
- Port 9443 already in use: `sudo lsof -i :9443`
- SSL cert paths incorrect in config
- Syntax error in nginx config

### Issue: 502 Bad Gateway
**Check:**
1. Is Chimera running? `ps aux | grep chimera`
2. Is it bound to 9001? `sudo netstat -tlnp | grep 9001`
3. Check Chimera logs for WebSocket errors

**Debug:**
```bash
# Test direct connection to Chimera
curl -i -N -H "Connection: Upgrade" \
     -H "Upgrade: websocket" \
     -H "Sec-WebSocket-Version: 13" \
     -H "Sec-WebSocket-Key: test" \
     http://127.0.0.1:9001/
```

Expected: `HTTP/1.1 101 Switching Protocols`

### Issue: GUI shows "--" for all fields
**Check browser console:**
- F12 → Console tab
- Look for WebSocket connection errors
- Check Network tab for failed requests

**Verify WebSocket connection:**
```javascript
// Paste in browser console:
let ws = new WebSocket("wss://localhost:9443/ws");
ws.onopen = () => console.log("Connected!");
ws.onmessage = (e) => console.log("Data:", e.data);
ws.onerror = (e) => console.error("Error:", e);
```

### Issue: "Unknown client spec version 0"
**This means:** nginx is NOT properly forwarding WebSocket headers

**Fix:** Check nginx config has:
```nginx
proxy_set_header Upgrade $http_upgrade;
proxy_set_header Connection "upgrade";
proxy_http_version 1.1;
```

### Issue: Port 9001 already in use
**Find process:**
```bash
sudo lsof -i :9001
sudo kill <PID>
```

**Or change port:**
1. Edit `src/main.cpp` line 19: `WsTelemetryServer server(9002, spine, "");`
2. Edit `config/nginx_chimera.conf` line 51: `proxy_pass http://127.0.0.1:9002;`
3. Rebuild and restart

## Production Deployment

### Security Checklist
- [ ] Use real SSL certificate (not self-signed)
- [ ] Configure firewall to only allow 9443 inbound
- [ ] Ensure Chimera binds only to 127.0.0.1 (not 0.0.0.0)
- [ ] Set up log rotation for nginx and Chimera
- [ ] Configure nginx rate limiting
- [ ] Enable nginx access logs for audit

### Monitoring
Monitor these endpoints:
- `https://hostname:9443/health` - nginx health check
- WebSocket connection count
- Chimera process memory/CPU usage

### Systemd Service (Optional)

Create `/etc/systemd/system/chimera.service`:
```ini
[Unit]
Description=ChimeraCrypto Trading Engine
After=network.target

[Service]
Type=simple
User=chimera
WorkingDirectory=/opt/chimera
ExecStart=/opt/chimera/build/chimera
Restart=always
RestartSec=10
StandardOutput=journal
StandardError=journal

[Install]
WantedBy=multi-user.target
```

Enable and start:
```bash
sudo systemctl daemon-reload
sudo systemctl enable chimera
sudo systemctl start chimera
sudo systemctl status chimera
```

## File Integrity Verification

To verify your deployment matches the fixed version:

```bash
# Check main.cpp includes WsTelemetryServer
grep "WsTelemetryServer.hpp" src/main.cpp
# Expected: #include "telemetry/WsTelemetryServer.hpp"

# Check app.js connects to /ws
grep "WebSocket.*location.host" gui/app.js
# Expected: new WebSocket("ws://"+location.host+"/ws");

# Check WsTelemetryServer header matches implementation
diff -u <(grep "WsTelemetryServer(" include/telemetry/WsTelemetryServer.hpp) \
        <(grep "WsTelemetryServer::" src/telemetry/WsTelemetryServer.cpp | head -1)
# Should show matching signatures
```

## Known Stable State

When properly deployed, the system should exhibit:

✅ **Zero** Content-Length mismatch errors  
✅ **Zero** 502 Bad Gateway responses  
✅ **Zero** "Unknown client spec version 0" logs  
✅ **Zero** port binding conflicts  
✅ Real-time WebSocket updates with <50ms latency  
✅ Automatic reconnection on network interruption  
✅ Clean separation: nginx (public), Chimera (localhost)  

## Architecture Guarantees

This configuration provides:
1. **No static file conflicts** - nginx serves, Chimera doesn't
2. **No port conflicts** - nginx on 9443, Chimera on 9001 localhost
3. **No protocol confusion** - WebSocket only, no mixed HTTP/WS
4. **No ABI mismatches** - headers and implementations aligned
5. **No half-migrations** - single transport model locked in

## Support

If issues persist after following this guide:
1. Check `FORENSIC_AUDIT.md` for architectural details
2. Verify all files match the checksums in MANIFEST.txt
3. Ensure build system has all dependencies installed
4. Check that no other process is using port 9001 or 9443

Last updated: 2026-02-20
Version: 1.0.0-fixed
