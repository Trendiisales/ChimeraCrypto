# ChimeraCrypto Forensic Audit Report
## Date: 2026-02-20

## Executive Summary
**STATUS**: CRITICAL ARCHITECTURAL INCONSISTENCY DETECTED

The codebase exhibits a half-migrated state where:
1. **main.cpp** references HttpTelemetryServer but the class is NOT in a header
2. **GUI JavaScript** expects pure WebSocket connection to root path
3. **WsTelemetryServer** exists but is NOT used in main.cpp
4. **Port configuration** is correct (9001) but transport layer is mismatched
5. **No nginx configuration** present in tarball

## Critical Issues Found

### Issue 1: HttpTelemetryServer Declaration Mismatch
**File**: `src/main.cpp` (line 22)
**Problem**: 
```cpp
chimera::HttpTelemetryServer server(9001, spine);
```
- HttpTelemetryServer is forward-declared but defined INLINE in HttpTelemetryServer.cpp
- No public header exists in include/telemetry/
- This violates C++ ODR (One Definition Rule)

**Consequence**: Potential linker errors or undefined behavior

### Issue 2: GUI WebSocket Configuration
**File**: `gui/app.js` (line 26)
**Problem**:
```javascript
let ws=new WebSocket("ws://"+location.host);
```
- Connects to root WebSocket path
- HttpTelemetryServer does NOT support WebSocket protocol
- HttpTelemetryServer only serves HTTP GET on /telemetry endpoint

**Consequence**: WebSocket connection will FAIL. GUI will not receive data.

### Issue 3: WsTelemetryServer Unused
**File**: `src/telemetry/WsTelemetryServer.cpp`
**Status**: COMPLETE and CORRECT implementation exists
**Problem**: main.cpp does NOT use it

The WsTelemetryServer:
- ✓ Properly matches header signature
- ✓ Correctly implements libwebsockets protocol
- ✓ Has proper thread management
- ✓ Sends JSON telemetry on WebSocket connection
- ✗ Is NEVER instantiated in main.cpp

### Issue 4: Missing nginx Configuration
**Expected**: `/etc/nginx/sites-available/chimera`
**Found**: NOT present in tarball

For production deployment, nginx should:
1. Bind to public port (443/9443)
2. Serve static files from gui/
3. Proxy /ws to 127.0.0.1:9001 for WebSocket
4. Handle SSL/TLS termination

## Architecture Analysis

### Current (Broken) State
```
Browser
  ↓
  Direct connection to localhost:9001
  ↓
  HttpTelemetryServer (HTTP only, no WebSocket)
  ↓
  GUI expects WebSocket → CONNECTION FAILS
```

### What Should Happen (Option A - WebSocket via nginx)
```
Browser (port 443/9443)
  ↓
  nginx
    ├─ Static files: / → gui/
    └─ WebSocket: /ws → 127.0.0.1:9001
       ↓
       WsTelemetryServer (libwebsockets)
       ↓
       TelemetrySpine
```

### What Should Happen (Option B - HTTP Polling)
```
Browser (port 443/9443)
  ↓
  nginx
    ├─ Static files: / → gui/
    └─ API: /telemetry → 127.0.0.1:9001
       ↓
       HttpTelemetryServer (plain HTTP)
       ↓
       TelemetrySpine

GUI modified to use fetch() polling instead of WebSocket
```

## Recommended Fixes

### Priority 1: Choose Architecture
**Decision Required**: WebSocket (real-time) OR HTTP polling (simpler)

### Priority 2: Fix main.cpp
If WebSocket:
```cpp
#include "telemetry/WsTelemetryServer.hpp"

int main() {
    // ... engine setup ...
    chimera::TelemetrySpine spine;
    chimera::WsTelemetryServer server(9001, spine, "");
    server.start();
    // ...
}
```

If HTTP polling:
- Create proper HttpTelemetryServer.hpp
- Move class definition to header
- Include in main.cpp

### Priority 3: Fix GUI
If WebSocket:
```javascript
let ws=new WebSocket("ws://"+location.host+"/ws");
```

If HTTP polling:
```javascript
setInterval(() => {
    fetch('/telemetry')
        .then(r => r.json())
        .then(update);
}, 250);
```

### Priority 4: Add nginx Configuration
Create `/etc/nginx/sites-available/chimera`:

**For WebSocket:**
```nginx
server {
    listen 9443 ssl http2;
    server_name _;
    
    ssl_certificate /path/to/cert.pem;
    ssl_certificate_key /path/to/key.pem;
    
    root /home/jo/ChimeraCrypto/gui;
    index index.html;
    
    location / {
        try_files $uri $uri/ =404;
    }
    
    location /ws {
        proxy_pass http://127.0.0.1:9001;
        proxy_http_version 1.1;
        proxy_set_header Upgrade $http_upgrade;
        proxy_set_header Connection "upgrade";
        proxy_set_header Host $host;
    }
}
```

**For HTTP Polling:**
```nginx
server {
    listen 9443 ssl http2;
    server_name _;
    
    ssl_certificate /path/to/cert.pem;
    ssl_certificate_key /path/to/key.pem;
    
    root /home/jo/ChimeraCrypto/gui;
    index index.html;
    
    location / {
        try_files $uri $uri/ =404;
    }
    
    location /telemetry {
        proxy_pass http://127.0.0.1:9001;
        proxy_set_header Host $host;
        add_header Access-Control-Allow-Origin *;
    }
}
```

## Files Requiring Modification

### Critical Path (WebSocket - Recommended)
1. `src/main.cpp` - Switch to WsTelemetryServer
2. `gui/app.js` - Connect to /ws endpoint
3. Add nginx config file to tarball
4. Document deployment steps

### Critical Path (HTTP Polling - Simpler)
1. Create `include/telemetry/HttpTelemetryServer.hpp`
2. Move HttpTelemetryServer class to header
3. Update `src/telemetry/HttpTelemetryServer.cpp` to match
4. Modify `gui/app.js` to use fetch() polling
5. Add nginx config file to tarball

## Verification Checklist

After fixes applied:
- [ ] Code compiles without warnings
- [ ] No forward-declaration-only classes in main.cpp
- [ ] GUI transport matches backend (WS or HTTP)
- [ ] Port 9001 not exposed to public (nginx proxy only)
- [ ] Static files served by nginx, not Chimera
- [ ] TelemetrySpine::json() provides all fields GUI expects
- [ ] No ABI mismatches between headers and implementations

## Conclusion

The current tarball is in a **non-functional state** for GUI telemetry.

Root cause: **Mid-migration between HTTP and WebSocket transports**

The code quality of individual components is GOOD:
- WsTelemetryServer.cpp is properly implemented
- HttpTelemetryServer.cpp works correctly
- GUI HTML/CSS is fine
- TelemetrySpine is correct

The problem is **integration inconsistency**:
- main.cpp uses one transport
- GUI expects different transport
- nginx config missing entirely

**Recommended Fix**: Complete WebSocket migration (Option A) as it provides:
- Real-time updates without polling overhead
- Proper separation via nginx proxy
- Already-implemented WsTelemetryServer ready to use

**Estimated fix time**: 30 minutes
**Risk level**: LOW (isolated changes, clear dependencies)
