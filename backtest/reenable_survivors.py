#!/usr/bin/env python3
import csv
import re
import sys
from pathlib import Path

CSV = Path("/Users/jo/ChimeraCrypto/backtest/mass_validate_survivors.csv")
MAIN = Path("/Users/jo/ChimeraCrypto/src/main.cpp")

tags = set()
with open(CSV) as f:
    r = csv.DictReader(f)
    for row in r:
        tags.add(row["tag"])

src = MAIN.read_text().splitlines()
out = []
flipped = 0
not_found_for = set(tags)

pat = re.compile(r'^(\s*)// DISABLED-TOP5:\s*(g_slots\.push_back\([^)]*"([^"]+)"[^)]*\);.*)$')

for line in src:
    m = pat.match(line)
    if m and m.group(3) in tags:
        out.append(f'{m.group(1)}{m.group(2)}')
        flipped += 1
        not_found_for.discard(m.group(3))
    else:
        out.append(line)

MAIN.write_text("\n".join(out) + "\n")
print(f"Flipped {flipped} lines")
print(f"Survivor tags not matched in main.cpp: {len(not_found_for)}")
for t in sorted(not_found_for):
    print(f"  MISSED: {t}")
