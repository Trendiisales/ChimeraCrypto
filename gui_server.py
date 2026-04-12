#!/usr/bin/env python3
"""
gui_server.py — DISABLED.

The C++ engine (QuadEngineBalancedEngine) owns the HTTP server on port 8080.
It serves /api/state, /api/kill, /api/flatten, and all GUI static files directly.

This file previously ran a Python HTTP server on port 8080 which conflicted with
the C++ server — whichever started first won the port, causing the other to fail
silently. The C++ server is authoritative: it has the live trade_log, real-time
per-symbol state, and all GUI endpoints.

DO NOT start this file. It is kept only for reference.
If you need a fallback debug server, change port to 8081 first.
"""
import sys
print("[gui_server] This file is disabled. The C++ engine owns port 8080.")
print("[gui_server] Run the chimera binary instead.")
sys.exit(0)
