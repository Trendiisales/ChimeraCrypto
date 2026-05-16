import json

# All engine data from the batch runs
engines_raw = """ethusdt|RSI_REVERT|H8|5|20|4.0|1.2|1.0|1.70|0.96|100|49
bnbusdt|RSI_REVERT|H8|8|6|2.0|1.0|0.8|1.95|1.07|96|31
dogeusdt|RSI_REVERT|H8|8|24|2.5|2.0|0.5|1.80|1.22|100|20
xrpusdt|RSI_REVERT|H8|35|8|3.0|0.5|0.3|14.85|2.01|98|11
aptusdt|RSI_REVERT|H8|30|8|3.0|1.5|0.3|3.03|1.27|68|11
solusdt|RSI_REVERT|H8|30|16|3.5|1.2|0.6|2.97|1.09|67|10
linkusdt|RSI_REVERT|H8|30|12|2.5|0.8|0.3|4.50|1.77|44|12
arbusdt|RSI_REVERT|H8|30|6|2.5|1.0|0.3|1.56|0.80|53|14
nearusdt|RSI_REVERT|H8|8|8|2.0|1.5|1.0|1.33|0.56|45|40
btcusdt|BOLLINGER|H8|8|8|4.0|1.2|0.3|2.02|0.93|44|20
ethusdt|BOLLINGER|H8|25|24|1.5|2.0|0.4|1.51|0.80|66|35
solusdt|BOLLINGER|H8|10|8|2.0|0.8|0.3|4.44|1.90|81|11
bnbusdt|BOLLINGER|H8|10|20|1.5|0.5|0.3|5.10|2.64|45|12
avaxusdt|BOLLINGER|H8|25|20|2.0|0.5|0.3|2.01|0.93|46|10
linkusdt|BOLLINGER|H8|30|20|3.0|0.5|0.3|6.83|2.70|86|24
xrpusdt|BOLLINGER|H8|25|10|3.5|0.5|0.4|2.56|1.45|62|34
dogeusdt|BOLLINGER|H8|25|20|3.0|0.5|0.3|4.24|2.11|50|14
suiusdt|BOLLINGER|H8|25|10|1.5|0.5|0.3|2.49|1.55|66|13
aptusdt|BOLLINGER|H8|35|6|2.0|0.8|0.6|1.96|0.91|61|11
nearusdt|BOLLINGER|H8|20|24|4.0|2.0|0.8|2.54|1.31|83|17
arbusdt|BOLLINGER|H8|20|10|2.0|0.5|0.3|2.53|1.58|64|12
ethusdt|RSI_REVERT|H16|5|24|4.0|1.0|0.3|158.17|9.07|100|24
bnbusdt|RSI_REVERT|H16|8|24|2.0|2.0|0.3|3.87|1.78|100|10
xrpusdt|RSI_REVERT|H16|8|24|3.0|0.8|0.3|3.71|1.41|100|20
linkusdt|RSI_REVERT|H16|15|24|4.0|0.8|0.6|2.37|1.13|72|17
nearusdt|RSI_REVERT|H16|5|24|2.5|0.8|0.5|2.02|0.86|83|14
btcusdt|RSI_REVERT|H16|20|24|4.0|0.8|0.3|3.50|1.42|54|15
solusdt|RSI_REVERT|H16|15|24|2.5|1.2|0.3|4.29|2.17|46|14
dogeusdt|RSI_REVERT|H16|15|12|3.0|0.5|0.3|2.01|1.16|43|17
linkusdt|BOLLINGER|H16|10|24|3.0|0.5|0.4|6.77|2.24|100|10
xrpusdt|BOLLINGER|H16|35|24|4.0|0.8|0.3|3.01|0.90|85|11
btcusdt|BOLLINGER|H16|5|24|3.0|2.0|0.3|2.66|1.15|70|13
nearusdt|BOLLINGER|H16|35|24|2.0|1.2|0.5|2.94|1.15|55|10
ethusdt|BOLLINGER|H16|10|24|3.5|0.5|0.4|2.61|0.88|40|10
xrpusdt|DONCHIAN|H8|20|10|4.0|2.0|0.3|3.05|2.21|100|45
nearusdt|DONCHIAN|H8|15|20|4.0|0.5|0.3|2.43|2.13|73|55
suiusdt|DONCHIAN|H8|35|8|4.0|1.0|0.3|5.02|2.18|100|11
btcusdt|DONCHIAN|H8|30|20|3.5|0.5|0.3|1.48|0.78|44|61
arbusdt|DONCHIAN|H8|40|6|1.5|2.0|0.3|2.00|1.22|51|10
xrpusdt|DONCHIAN|H16|35|8|4.0|1.0|0.4|4.88|1.88|100|19
bnbusdt|DONCHIAN|H16|40|6|3.0|0.5|0.4|9.25|2.99|65|16
btcusdt|DONCHIAN|H16|40|20|3.5|0.5|0.3|2.48|1.27|67|28
linkusdt|DONCHIAN|H16|40|10|3.5|0.5|0.3|2.69|1.03|71|18
suiusdt|DONCHIAN|H16|15|10|2.5|0.8|0.3|3.68|1.86|67|10
nearusdt|DONCHIAN|H16|8|24|2.0|1.0|0.4|1.87|1.36|58|29
solusdt|DONCHIAN|H16|35|24|3.5|0.8|0.3|2.08|0.95|49|14
dogeusdt|DONCHIAN|H16|30|4|4.0|1.2|0.6|2.07|0.91|41|10
bnbusdt|DONCHIAN|D2|5|24|4.0|0.5|0.3|99.90|5.55|93|14
xrpusdt|DONCHIAN|D2|20|8|1.5|0.5|0.4|10.03|1.90|100|10
btcusdt|DONCHIAN|D2|15|24|4.0|0.8|0.5|5.56|1.55|84|12
ethusdt|DONCHIAN|D2|25|20|4.0|0.5|0.3|3.29|1.04|95|10
linkusdt|DONCHIAN|D2|20|4|3.5|0.5|0.3|2.78|0.95|83|10
btcusdt|DONCHIAN|D3|10|10|2.5|0.5|0.3|128.98|2.40|98|12
ethusdt|DONCHIAN|D3|20|4|4.0|0.5|0.3|9.21|2.16|87|10
xrpusdt|DONCHIAN|D3|5|8|3.0|1.2|1.0|5.39|1.41|100|10
bnbusdt|DONCHIAN|D3|5|4|1.5|2.0|0.3|2.09|0.97|80|11
dogeusdt|DONCHIAN|D3|5|4|1.5|0.5|0.3|1.97|0.82|96|8"""

TF_SECS = {"H6":21600,"H8":28800,"H12":43200,"H16":57600,"D1":86400,"D2":172800,"D3":259200}
SYM_ENUM = {"btcusdt":"BTC","ethusdt":"ETH","solusdt":"SOL","bnbusdt":"BNB","avaxusdt":"AVAX","linkusdt":"LINK","xrpusdt":"XRP","dogeusdt":"DOGE","suiusdt":"SUI","aptusdt":"APT","nearusdt":"NEAR","arbusdt":"ARB"}
SYM_PREFIX = {"btcusdt":"btc","ethusdt":"eth","solusdt":"sol","bnbusdt":"bnb","avaxusdt":"avax","linkusdt":"link","xrpusdt":"xrp","dogeusdt":"doge","suiusdt":"sui","aptusdt":"apt","nearusdt":"near","arbusdt":"arb"}
STRAT_KIND = {"TSMOM":"chimera::StrategyKind::TSMOM","RSI_REVERT":"chimera::StrategyKind::RSI_REVERT","BOLLINGER":"chimera::StrategyKind::BOLLINGER","DONCHIAN":"chimera::StrategyKind::DONCHIAN"}
STRAT_SHORT = {"TSMOM":"tsmom","RSI_REVERT":"rsi","BOLLINGER":"boll","DONCHIAN":"donch"}
STRAT_TAG = {"TSMOM":"TSMOM","RSI_REVERT":"RSI","BOLLINGER":"BOLL","DONCHIAN":"DONCH"}

configs = []
gslots = []
spreadsheet_rows = []

for line in engines_raw.strip().split("\n"):
    parts = line.split("|")
    sym, strat, tf = parts[0], parts[1], parts[2]
    lb, hb = int(parts[3]), int(parts[4])
    sl, ta, td = float(parts[5]), float(parts[6]), float(parts[7])
    pf, sharpe = float(parts[8]), float(parts[9])
    nbr, trades = int(parts[10]), int(parts[11])
    
    prefix = SYM_PREFIX[sym]
    short = STRAT_SHORT[strat]
    tag = f"{SYM_ENUM[sym]}-{STRAT_TAG[strat]}-{tf}"
    var = f"{prefix}_{short}_{tf.lower()}"
    tf_s = TF_SECS[tf]
    
    # Config block
    cfg = f"""    chimera::EdgeEngine::Config {var}_cfg{{
        .symbol         = "{sym}",
        .tag            = "{tag}",
        .kind           = {STRAT_KIND[strat]},
        .tf_secs        = {tf_s},
        .lookback       = {lb},
        .hold_bars      = {hb},
        .sl_atr_mult    = {sl:.1f},
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22.0,
        .max_history    = 64,
        .trail_arm_atr  = {ta:.1f},
        .trail_dist_atr = {td:.1f},
    }};
    chimera::EdgeEngine {var}({var}_cfg);
    {var}.set_on_trade(on_trade_callback);"""
    configs.append(cfg)
    
    # g_slots line
    gslot = f'    g_slots.push_back({{chimera::SYM_{SYM_ENUM[sym]}, &{var}, "{sym}", {tf_s}, "{tag}", {pf:.2f}, {sharpe:.2f}, {nbr}, {trades}, 22}});'
    gslots.append(gslot)
    
    # Spreadsheet row
    spreadsheet_rows.append(f"{tag}|{SYM_ENUM[sym]}|{STRAT_TAG[strat]}|{tf}|{lb}|{hb}|{sl}|{ta}|{td}|22|{pf}|{sharpe}|{nbr}|{trades}|S22|Shadow")

# Write files
with open("new_engines_configs.cpp", "w") as f:
    f.write("\n\n".join(configs))

with open("new_engines_gslots.cpp", "w") as f:
    f.write("\n".join(gslots))

with open("new_engines_spreadsheet.txt", "w") as f:
    f.write("Tag|Symbol|Strategy|TF|LB|HB|SL ATR|Trail Arm|Trail Dist|Cost bp|PF|Sharpe|Nbr%|OOS Trades|Session|Grade\n")
    for row in spreadsheet_rows:
        f.write(row + "\n")

print(f"Generated {len(configs)} engine configs")
print(f"Generated {len(gslots)} g_slots entries")
