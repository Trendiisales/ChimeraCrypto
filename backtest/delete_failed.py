#!/usr/bin/env python3
"""Remove failed engines from main.cpp.

For each FAIL tag: delete Config block, EdgeEngine instance line, wire_engine
call, and the commented DISABLED-TOP5 push_back. Optionally remove preceding
single-line `// ENGINE ...: <TAG>` comment.
"""
import csv
import json
import re
from pathlib import Path

MAIN = Path("/Users/jo/ChimeraCrypto/src/main.cpp")
CONFIGS = json.load(open("/Users/jo/ChimeraCrypto/backtest/disabled_engines.json"))
RESULTS = list(csv.DictReader(open("/Users/jo/ChimeraCrypto/backtest/mass_validate_results.csv")))

cfg_by_tag = {c["tag"]: c for c in CONFIGS}
fail_tags = [r["tag"] for r in RESULTS if r["verdict"] == "FAIL"]

lines = MAIN.read_text().splitlines(keepends=True)
to_delete = set()  # 0-indexed line indices

stats = {"cfg_blocks": 0, "engine_inst": 0, "wire": 0, "push_back": 0, "header_cmt": 0}
missing = []

for tag in fail_tags:
    c = cfg_by_tag.get(tag)
    if not c:
        missing.append(tag)
        continue
    var = c["var"]
    cfg_var = c["cfg_var"]

    # 1. Config block: line "chimera::EdgeEngine::Config <cfg_var>{"
    #    through next line that is exactly "};" (possibly preceded by spaces).
    cfg_start_pat = re.compile(rf"^\s*chimera::EdgeEngine::Config\s+{re.escape(cfg_var)}\s*\{{")
    cfg_end_pat = re.compile(r"^\s*\};\s*$")
    cfg_start = None
    for i, ln in enumerate(lines):
        if cfg_start_pat.match(ln):
            cfg_start = i
            break
    if cfg_start is not None:
        j = cfg_start
        while j < len(lines) and not cfg_end_pat.match(lines[j]):
            j += 1
        if j < len(lines):
            for k in range(cfg_start, j + 1):
                to_delete.add(k)
            stats["cfg_blocks"] += 1
            # preceding `// ENGINE ...:` comment line
            if cfg_start > 0 and re.match(r"^\s*//\s*ENGINE\b", lines[cfg_start - 1]):
                to_delete.add(cfg_start - 1)
                stats["header_cmt"] += 1

    # 2. EdgeEngine instance: "chimera::EdgeEngine <var>(<cfg_var>);"
    inst_pat = re.compile(rf"^\s*chimera::EdgeEngine\s+{re.escape(var)}\s*\(\s*{re.escape(cfg_var)}\s*\)\s*;\s*$")
    for i, ln in enumerate(lines):
        if inst_pat.match(ln):
            to_delete.add(i)
            stats["engine_inst"] += 1
            break

    # 3. wire_engine call
    wire_pat = re.compile(rf"^\s*wire_engine\s*\(\s*{re.escape(var)}\s*\)\s*;\s*$")
    for i, ln in enumerate(lines):
        if wire_pat.match(ln):
            to_delete.add(i)
            stats["wire"] += 1
            break

    # 4. DISABLED-TOP5 commented push_back
    pb_pat = re.compile(rf'^\s*//\s*DISABLED-TOP5:.*"{re.escape(tag)}".*$')
    for i, ln in enumerate(lines):
        if pb_pat.match(ln):
            to_delete.add(i)
            stats["push_back"] += 1
            break

out_lines = [ln for i, ln in enumerate(lines) if i not in to_delete]
MAIN.write_text("".join(out_lines))

print(f"FAIL tags processed: {len(fail_tags)}")
print(f"Missing in JSON:     {len(missing)}")
print(f"Lines removed:       {len(to_delete)}")
print(f"  Config blocks:     {stats['cfg_blocks']}")
print(f"  Engine instances:  {stats['engine_inst']}")
print(f"  wire_engine calls: {stats['wire']}")
print(f"  push_back lines:   {stats['push_back']}")
print(f"  Header comments:   {stats['header_cmt']}")
for t in missing:
    print(f"  MISSING: {t}")
