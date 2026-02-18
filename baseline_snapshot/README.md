# ChimeraCrypto Baseline Snapshot

This directory contains the original baseline implementation of ChimeraCrypto before enhancements.

## Contents

- **src/**: Original source files
  - `main.cpp`: Basic test harness
  - `InstitutionalEngine.cpp`: Minimal implementation
  
- **include/**: Original header files
  - `InstitutionalEngine.hpp`: Engine interface
  - `ExecutionTracker.hpp`: Basic enums
  
- **CMakeLists.txt**: Original build configuration

## Baseline Functionality

The baseline implementation provides:
- Simple equity tracking (increments by $1 per tick)
- Stub methods for order book updates and trade recording
- Basic threading with sleep delays
- Minimal demonstration in main.cpp

## Build Instructions

```bash
mkdir build
cd build
cmake ..
make
./chimera
```

Expected output:
```
Final Equity: $10005.00
```

## Purpose

This snapshot preserves the original implementation for:
- Reference and comparison
- Understanding the evolution of the system
- Regression testing
- Educational purposes

The current main branch contains the full production implementation with all enhancements.
