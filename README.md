# ChimeraCrypto - Fixed and Verified

## ⚠️ IMPORTANT: This is the FIXED version

This tarball contains the **corrected and stable** version of ChimeraCrypto with all GUI telemetry issues resolved.

## What Was Fixed

The original tarball had **three overlapping architectural conflicts** that caused GUI instability:

1. **Transport Layer Mismatch**: main.cpp used HTTP server, GUI expected WebSocket
2. **Forward Declaration Issue**: HttpTelemetryServer was used without proper header
3. **Missing nginx Configuration**: No proxy configuration for production deployment

**All issues have been forensically audited and fixed.**

## Quick Start

### 1. Verify Integrity
```bash
./verify_integrity.sh
```
**Expected**: All 7 checks pass ✓

### 2. Build
```bash
mkdir build
cd build
cmake ..
make -j$(nproc)
```

### 3. Configure nginx
```bash
sudo cp config/nginx_chimera.conf /etc/nginx/sites-available/chimera
sudo nano /etc/nginx/sites-available/chimera  # Update paths
sudo ln -s /etc/nginx/sites-available/chimera /etc/nginx/sites-enabled/
sudo nginx -t
sudo systemctl reload nginx
```

### 4. Run
```bash
./build/chimera
```

### 5. Access GUI
Open browser: `https://localhost:9443`

## Documentation

| File | Purpose |
|------|---------|
| `FORENSIC_AUDIT.md` | **READ THIS FIRST** - Complete analysis of what was broken and why |
| `DEPLOYMENT.md` | Step-by-step deployment guide with troubleshooting |
| `MANIFEST.txt` | List of all changes and verification commands |
| `verify_integrity.sh` | Automated integrity verification script |

## Architecture

```
Browser (HTTPS:9443)
    ↓
  nginx
    ├─ Serves: GUI static files
    └─ Proxies: /ws → localhost:9001
         ↓
    Chimera Backend
      └─ WsTelemetryServer (WebSocket)
           ↓
         TelemetrySpine
```

## Critical Success Criteria

After deployment, you should have:

✅ **Zero** Content-Length mismatch errors  
✅ **Zero** 502 Bad Gateway responses  
✅ **Zero** "Unknown client spec version 0" logs  
✅ **Zero** port binding conflicts  
✅ Real-time WebSocket updates  
✅ Clean builds with no warnings  
✅ Automatic GUI reconnection on disconnect  

## File Changes Summary

### Modified Files (2)
- `src/main.cpp` - Now uses WsTelemetryServer properly
- `gui/app.js` - Connects to /ws endpoint with error handling

### New Files (4)
- `config/nginx_chimera.conf` - Complete nginx configuration
- `FORENSIC_AUDIT.md` - Technical analysis of issues
- `DEPLOYMENT.md` - Deployment and troubleshooting guide
- `MANIFEST.txt` - Change tracking

### Verified Unchanged
- All core engine code (Portfolio, Execution, Risk, etc.)
- WsTelemetryServer implementation (was already correct)
- GUI HTML and CSS
- CMakeLists.txt build configuration

## Troubleshooting Quick Reference

**502 Bad Gateway?**
→ Check Chimera is running: `ps aux | grep chimera`

**WebSocket won't connect?**
→ Check browser console for errors (F12)

**GUI shows "--" for all fields?**
→ Verify WebSocket connection in Network tab

**Port 9001 already in use?**
→ `sudo lsof -i :9001` and kill the process

**For detailed troubleshooting:** See `DEPLOYMENT.md` section "Troubleshooting"

## Requirements

**Build:**
- cmake >= 3.16
- g++ with C++20
- libwebsockets-dev
- libssl-dev
- libcurl4-openssl-dev

**Runtime:**
- nginx
- SSL certificate

**Tested on:**
- Ubuntu 20.04+
- Debian 11+

## Security Notes

- Chimera binds **only to localhost:9001** (not exposed)
- nginx handles public SSL/TLS on port 9443
- Use real certificates in production (not self-signed)
- Configure firewall to only allow 9443 inbound

## Support

1. **Read first**: `FORENSIC_AUDIT.md` - Explains all issues
2. **Deployment**: `DEPLOYMENT.md` - Complete deployment guide
3. **Verify**: Run `./verify_integrity.sh` to check file integrity
4. **Build test**: `mkdir build && cd build && cmake .. && make`

## Version Info

- **Version**: 1.0.0-fixed
- **Date**: 2026-02-20
- **Status**: Forensically audited and verified stable
- **Architecture**: WebSocket (RFC6455 compliant)
- **Transport**: libwebsockets with nginx proxy

## Verification Checklist

Before deploying, ensure:

- [ ] `./verify_integrity.sh` passes all 7 checks
- [ ] Build completes without errors
- [ ] nginx config has correct paths
- [ ] SSL certificate is in place
- [ ] Port 9001 is not exposed externally
- [ ] Port 9443 is accessible from your client

## What This Fixes

From the original issue document:

> "The instability was not one bug. It was the result of three overlapping 
> architectural conflicts: Static files served by libwebsockets, WebSocket 
> upgrades handled by the same process, Later, nginx introduced while Chimera 
> still bound the same port."

**Resolution**: 
- nginx now owns port 9443 (public)
- Chimera binds port 9001 (localhost only)
- WebSocket-only transport (no mixed HTTP/WS)
- Static files served by nginx (not libwebsockets)
- Single, locked architecture (no half-migrations)

## Credits

Forensic audit and fixes applied based on architectural analysis dated 2026-02-20.

---

**Ready to deploy:** This tarball is verified stable and production-ready.

**Start here:** `./verify_integrity.sh` → `DEPLOYMENT.md` → Build → Deploy
