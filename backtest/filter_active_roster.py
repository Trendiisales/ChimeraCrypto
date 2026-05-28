#!/usr/bin/env python3
import re, csv, pathlib
SRC = pathlib.Path("/Users/jo/ChimeraCrypto/src/main.cpp").read_text()
IN_CSV = pathlib.Path("/Users/jo/ChimeraCrypto/backtest/engine_roster.csv")
OUT_CSV = pathlib.Path("/Users/jo/ChimeraCrypto/backtest/engine_roster_active.csv")

active_tags = set()
for m in re.finditer(r'^\s*g_slots\.push_back\([^)]*"([A-Z0-9\-]+)"', SRC, re.MULTILINE):
    active_tags.add(m.group(1))

with IN_CSV.open() as f, OUT_CSV.open("w", newline="") as o:
    r = csv.DictReader(f)
    w = csv.DictWriter(o, fieldnames=r.fieldnames)
    w.writeheader()
    kept = 0
    for row in r:
        if row["tag"] in active_tags:
            w.writerow(row); kept += 1
print(f"Active tags in g_slots: {len(active_tags)}")
print(f"Roster active rows: {kept}")
