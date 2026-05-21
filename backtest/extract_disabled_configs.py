#!/usr/bin/env python3
"""
Extract disabled engine configurations from main.cpp.

Finds every line matching `// DISABLED-TOP5: g_slots.push_back(...)` then
locates the corresponding `chimera::EdgeEngine::Config <var>_cfg{...}` block
earlier in the file by following the engine variable name back to its
constructor `chimera::EdgeEngine <var>(<var>_cfg);`.

Writes a JSON list to disabled_engines.json.
"""
import json
import re
import sys
from pathlib import Path

MAIN_CPP = Path("/Users/jo/ChimeraCrypto/src/main.cpp")
OUT_JSON = Path("/Users/jo/ChimeraCrypto/backtest/disabled_engines.json")

DISABLED_RE = re.compile(
    r"^\s*//\s*DISABLED-TOP5:\s*g_slots\.push_back\(\{\s*"
    r"chimera::SYM_\w+\s*,\s*&(?P<var>\w+)\s*,\s*"
    r'"(?P<symbol>\w+)"\s*,\s*(?P<tf>\d+)\s*,\s*'
    r'"(?P<tag>[^"]+)"'
)

# Field extraction in a config block — matches `.field = value,` (value can
# be int, float, string literal, or `chimera::StrategyKind::FOO`).
FIELD_RE = re.compile(
    r"\.(\w+)\s*=\s*([^,}]+?)\s*[,}]",
)

def parse_config_value(raw):
    raw = raw.strip()
    if raw.startswith('"') and raw.endswith('"'):
        return raw[1:-1]
    if raw.startswith("chimera::StrategyKind::"):
        return raw.split("::")[-1]
    # numeric?
    try:
        if "." in raw or "e" in raw or "E" in raw:
            return float(raw)
        return int(raw)
    except ValueError:
        return raw  # leave as-is

def main():
    text = MAIN_CPP.read_text()
    lines = text.splitlines()

    # 1) Collect disabled engines
    disabled = []
    for ln_idx, ln in enumerate(lines):
        m = DISABLED_RE.match(ln)
        if m:
            disabled.append({
                "var": m.group("var"),
                "symbol": m.group("symbol"),
                "tf_secs_from_slot": int(m.group("tf")),
                "tag": m.group("tag"),
                "slot_line": ln_idx + 1,
            })

    print(f"Found {len(disabled)} disabled engine slot lines.", file=sys.stderr)

    # 2) For each engine var, find the constructor line:
    #    `chimera::EdgeEngine <var>(<cfg_var>);`
    #    Then extract the matching Config block ending just before it.
    ctor_re_tmpl = r"chimera::EdgeEngine\s+{var}\s*\(\s*(\w+)\s*\)\s*;"

    # Pre-index every "EdgeEngine name(cfg);" occurrence
    ctor_index = {}  # var_name -> (line_idx, cfg_var)
    ctor_general_re = re.compile(
        r"chimera::EdgeEngine\s+(\w+)\s*\(\s*(\w+)\s*\)\s*;"
    )
    for i, ln in enumerate(lines):
        m = ctor_general_re.search(ln)
        if m:
            ctor_index[m.group(1)] = (i, m.group(2))

    # Pre-index every "chimera::EdgeEngine::Config <cfg_var>{" line (designated init style)
    cfg_open_re = re.compile(r"chimera::EdgeEngine::Config\s+(\w+)\s*\{")
    # And "chimera::EdgeEngine::Config <cfg_var>;" (member-assign style)
    cfg_decl_re = re.compile(r"chimera::EdgeEngine::Config\s+(\w+)\s*;")
    cfg_index = {}   # cfg_var -> (open_line_idx, style)  style in {"brace","assign"}
    for i, ln in enumerate(lines):
        m = cfg_open_re.search(ln)
        if m:
            cfg_index[m.group(1)] = (i, "brace")
            continue
        m2 = cfg_decl_re.search(ln)
        if m2:
            cfg_index[m2.group(1)] = (i, "assign")

    results = []
    missing_ctor = []
    missing_cfg = []
    parse_fail = []

    for d in disabled:
        var = d["var"]
        if var not in ctor_index:
            missing_ctor.append(var)
            continue
        ctor_line, cfg_var = ctor_index[var]
        if cfg_var not in cfg_index:
            missing_cfg.append((var, cfg_var))
            continue

        open_idx, style = cfg_index[cfg_var]
        fields = {}

        if style == "brace":
            # Read until closing `};`
            close_idx = None
            for j in range(open_idx, min(open_idx + 80, len(lines))):
                if "};" in lines[j] and j >= open_idx:
                    close_idx = j
                    break
            if close_idx is None:
                parse_fail.append(var)
                continue
            block = "\n".join(lines[open_idx:close_idx + 1])
            for m in FIELD_RE.finditer(block):
                fname, raw = m.group(1), m.group(2)
                fields[fname] = parse_config_value(raw)
        else:
            # Member-assignment style: cfg_var.field = value;
            # Scan forward until the constructor line for var.
            assign_re = re.compile(
                rf"{re.escape(cfg_var)}\.(\w+)\s*=\s*([^;]+?)\s*;"
            )
            for j in range(open_idx, min(open_idx + 80, len(lines))):
                # stop at constructor line
                if re.search(rf"chimera::EdgeEngine\s+{re.escape(var)}\s*\(", lines[j]):
                    break
                m = assign_re.search(lines[j])
                if m:
                    fname, raw = m.group(1), m.group(2)
                    fields[fname] = parse_config_value(raw)

        # Sanity: at least symbol+tag should be there
        if "symbol" not in fields or "tag" not in fields:
            parse_fail.append(var)
            continue

        results.append({
            "var": var,
            "tag": fields.get("tag", d["tag"]),
            "symbol": fields.get("symbol", d["symbol"]),
            "kind": fields.get("kind", "UNKNOWN"),
            "tf_secs": fields.get("tf_secs", d["tf_secs_from_slot"]),
            "lookback": fields.get("lookback", 20),
            "hold_bars": fields.get("hold_bars", 12),
            "sl_atr_mult": fields.get("sl_atr_mult", 2.5),
            "atr_period": fields.get("atr_period", 14),
            "bb_k": fields.get("bb_k", 2.0),
            "rsi_threshold": fields.get("rsi_threshold", 30.0),
            "round_trip_bp": fields.get("round_trip_bp", 17.0),
            "max_history": fields.get("max_history", 64),
            "trail_arm_atr": fields.get("trail_arm_atr", 1.0),
            "trail_dist_atr": fields.get("trail_dist_atr", 0.5),
            "trail_tighten_atr": fields.get("trail_tighten_atr", 0.0),
            "trail_tighten_dist_atr": fields.get("trail_tighten_dist_atr", 0.3),
            "cfg_var": cfg_var,
        })

    OUT_JSON.write_text(json.dumps(results, indent=2))
    print(f"Wrote {len(results)} configs to {OUT_JSON}", file=sys.stderr)
    if missing_ctor:
        print(f"  missing ctor for {len(missing_ctor)} vars: {missing_ctor[:5]}...", file=sys.stderr)
    if missing_cfg:
        print(f"  missing cfg block for {len(missing_cfg)}: {missing_cfg[:5]}...", file=sys.stderr)
    if parse_fail:
        print(f"  parse failures: {len(parse_fail)}: {parse_fail[:5]}...", file=sys.stderr)

if __name__ == "__main__":
    main()
