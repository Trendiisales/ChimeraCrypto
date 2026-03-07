// ── CHIMERA app.js ────────────────────────────────────────────────────────────
// Bell fix: only fire audio/flash for trades that arrive AFTER this page load.
// Trades already in localStorage from prior sessions are loaded silently.

const STORAGE_KEY = 'chimera_trades_v3';
const BOOT_TS = Date.now(); // trades with server-time < BOOT_TS are "old" — no bell

let localTrades = [];
let audioCtx = null;
let audioUnlocked = false;
let lastPrices = { btc: 0, eth: 0, sol: 0 };
let wins = 0, losses = 0;
let uptimeStart = null;

// ── AUDIO ─────────────────────────────────────────────────────────────────────
function unlockAudio() {
  if (audioUnlocked) return;
  try {
    audioCtx = new (window.AudioContext || window.webkitAudioContext)();
    // Play a silent buffer to unlock
    const buf = audioCtx.createBuffer(1, 1, 22050);
    const src = audioCtx.createBufferSource();
    src.buffer = buf;
    src.connect(audioCtx.destination);
    src.start(0);
    audioUnlocked = true;
    const btn = document.getElementById('audio-unlock');
    if (btn) { btn.textContent = '🔔 MUTED OFF'; btn.style.background = 'rgba(0,230,118,.1)'; btn.style.color = 'var(--green)'; btn.style.borderColor = 'var(--green)'; }
    console.log('[CHIMERA] Audio unlocked');
  } catch(e) { console.warn('Audio unlock failed:', e); }
}

function playWin() {
  if (!audioUnlocked || !audioCtx) return;
  try {
    // Rising two-tone chime — unmistakable win sound
    const now = audioCtx.currentTime;
    [[880, 0, 0.12], [1320, 0.1, 0.18]].forEach(([freq, delay, dur]) => {
      const osc = audioCtx.createOscillator();
      const gain = audioCtx.createGain();
      osc.connect(gain); gain.connect(audioCtx.destination);
      osc.type = 'sine'; osc.frequency.setValueAtTime(freq, now + delay);
      gain.gain.setValueAtTime(0, now + delay);
      gain.gain.linearRampToValueAtTime(0.4, now + delay + 0.02);
      gain.gain.exponentialRampToValueAtTime(0.001, now + delay + dur);
      osc.start(now + delay); osc.stop(now + delay + dur + 0.05);
    });
  } catch(e) {}
}

function playLoss() {
  if (!audioUnlocked || !audioCtx) return;
  try {
    const now = audioCtx.currentTime;
    const osc = audioCtx.createOscillator();
    const gain = audioCtx.createGain();
    osc.connect(gain); gain.connect(audioCtx.destination);
    osc.type = 'sawtooth'; osc.frequency.setValueAtTime(220, now);
    osc.frequency.linearRampToValueAtTime(110, now + 0.2);
    gain.gain.setValueAtTime(0.2, now);
    gain.gain.exponentialRampToValueAtTime(0.001, now + 0.25);
    osc.start(now); osc.stop(now + 0.3);
  } catch(e) {}
}

function flashWin(sym, pnl) {
  const el = document.getElementById('win-flash');
  if (!el) return;
  el.textContent = `✓ ${sym}  +${(+pnl).toFixed(2)}bp`;
  el.classList.add('show');
  setTimeout(() => el.classList.remove('show'), 2200);
}

// ── STORAGE ───────────────────────────────────────────────────────────────────
function loadTrades() {
  try {
    const raw = localStorage.getItem(STORAGE_KEY);
    localTrades = raw ? JSON.parse(raw) : [];
  } catch(e) { localTrades = []; }
}

function saveTrades() {
  try { localStorage.setItem(STORAGE_KEY, JSON.stringify(localTrades.slice(0, 200))); } catch(e) {}
}

window.clearTrades = function() {
  localTrades = []; wins = 0; losses = 0;
  localStorage.removeItem(STORAGE_KEY);
  renderTradeLog(); updateWinRate();
};

// ── TRADE MERGE ───────────────────────────────────────────────────────────────
// KEY FIX: isBootLoad=true means we are merging trades from localStorage on startup.
// Those trades never trigger bell/flash. Only server trades arriving after BOOT_TS do.
function mergeTrades(serverLog, isBootLoad) {
  if (!serverLog || !serverLog.length) return;
  const before = localTrades.length;
  const existing = new Set(localTrades.map(t => `${t.t}|${t.s}|${t.e}|${t.p}`));
  let newCount = 0;

  serverLog.forEach(tr => {
    const key = `${tr.t}|${tr.s}|${tr.e}|${tr.p}`;
    if (!existing.has(key)) {
      localTrades.unshift(tr);
      existing.add(key);
      newCount++;

      if (!isBootLoad) {
        // This trade arrived live during this session — fire bell
        if (+tr.p > 0) { wins++; playWin(); flashWin(tr.s, tr.p); }
        else if (+tr.p < 0) { losses++; playLoss(); }
        else { wins++; } // breakeven counts as win for WR
      } else {
        // Silently restore win/loss counts from historical data
        if (+tr.p > 0) wins++;
        else if (+tr.p < 0) losses++;
      }
    }
  });

  if (newCount > 0 || before === 0) {
    localTrades = localTrades.slice(0, 200);
    saveTrades();
    renderTradeLog();
    renderSymbolTrades();
    updateWinRate();
  }
}

// ── HELPERS ───────────────────────────────────────────────────────────────────
const $ = id => document.getElementById(id);
const set = (id, val, cls) => { const el = $(id); if (!el) return; el.textContent = val; if (cls !== undefined) el.className = cls; };
const fmtPnl = v => (v >= 0 ? '+' : '') + (+v).toFixed(2) + 'bp';
const pnlCls = (base, v) => base + ' ' + (+v > 0 ? 'pos' : +v < 0 ? 'neg' : '');

// Convert bp to dollars: $10,000 account, 1bp = $1
const ACCOUNT_SIZE = 10000;
const bpToUsd = bp => (bp / 10000) * ACCOUNT_SIZE;
const fmtUsd = v => (v >= 0 ? '+$' : '-$') + Math.abs(v).toFixed(2);

function fmtHold(ms) {
  if (!ms || ms <= 0) return '--';
  if (ms < 1000) return ms + 'ms';
  if (ms < 60000) return (ms / 1000).toFixed(1) + 's';
  return Math.floor(ms / 60000) + 'm' + Math.floor((ms % 60000) / 1000) + 's';
}

function renderSymbolTrades() {
  const syms = ['BTC','ETH','SOL'];
  syms.forEach(sym => {
    const el = $('str-' + sym.toLowerCase());
    if (!el) return;
    const trades = localTrades.filter(t => (t.s||'').replace('USDT','') === sym).slice(0, 8);
    if (!trades.length) {
      el.className = 'sym-trades-empty';
      el.innerHTML = 'No trades yet';
      return;
    }
    el.className = '';
    el.innerHTML = trades.map(tr => {
      const pnl   = +tr.p || 0;
      const isWin = pnl >= 0;
      const usd   = bpToUsd(pnl);
      const rc    = reasonClass(tr.why || tr.reason || '');
      const en    = tr.en  ? fmtPrice(tr.en,  sym) : '--';
      const ex    = tr.ex  ? fmtPrice(tr.ex,  sym) : '--';
      const why   = (tr.why || tr.reason || '?').toUpperCase();
      const time  = tr.t   ? tr.t.substring(11,19) : '--';
      return `<div class="sym-trade-row ${isWin?'win':'loss'}">
        <span class="str-tag ${isWin?'win':'loss'}">${isWin?'WIN':'LOSS'}</span>
        <span class="str-pnl ${isWin?'pos':'neg'}">${fmtPnl(pnl)}</span>
        <span class="str-usd ${isWin?'pos':'neg'}">${fmtUsd(usd)}</span>
        <span class="str-eng">${tr.e||'--'}</span>
        <span class="str-val">${en}→${ex}</span>
        <span class="str-val">${fmtHold(tr.hold)}</span>
        <span class="str-badge ${rc}">${why}</span>
        <span class="str-time">${time}</span>
      </div>`;
    }).join('');
  });
}

function fmtPrice(p, sym) {
  if (!p || p <= 0) return '--';
  if (sym === 'sol' || sym === 'SOL') return '$' + (+p).toFixed(3);
  return '$' + (+p).toLocaleString('en-US', { minimumFractionDigits: 2, maximumFractionDigits: 2 });
}

function updateUptime() {
  if (!uptimeStart) return;
  const s = Math.floor((Date.now() - uptimeStart) / 1000);
  const h = Math.floor(s / 3600), m = Math.floor((s % 3600) / 60), sec = s % 60;
  const fmt = `${String(h).padStart(2,'0')}:${String(m).padStart(2,'0')}:${String(sec).padStart(2,'0')}`;
  set('tb-uptime', fmt); set('st-uptime', fmt);
}

function updateWinRate() {
  const t = wins + losses;
  const wr = t > 0 ? (wins / t * 100).toFixed(0) + '%' : '--%';
  set('ts-wr', wr);
  const tswr = $('ts-wr');
  if (tswr) tswr.style.color = t > 0 ? (wins >= losses ? 'var(--green)' : 'var(--red)') : '';
}

// ── TRADE CARDS ───────────────────────────────────────────────────────────────
function reasonClass(r) {
  if (!r) return 'timeout';
  const rl = r.toLowerCase();
  if (rl === 'tp') return 'tp';
  if (rl === 'sl') return 'sl';
  if (rl === 'trail') return 'trail';
  return 'timeout';
}

function makeRow(tr) {
  const pnl  = +tr.p || 0;
  const isWin = pnl >= 0;
  const usd  = bpToUsd(pnl);
  const rc   = reasonClass(tr.why || tr.reason || '');
  const sym  = (tr.s || '').replace('USDT', '');
  const mfe  = tr.mfe != null ? `<span class="tl-val pos">+${(+tr.mfe).toFixed(1)}bp</span>` : '<span class="tl-val">--</span>';
  const mae  = tr.mae != null ? `<span class="tl-val neg">${(+tr.mae).toFixed(1)}bp</span>` : '<span class="tl-val">--</span>';
  const en   = tr.en  ? fmtPrice(tr.en,  sym) : '--';
  const ex   = tr.ex  ? fmtPrice(tr.ex,  sym) : '--';
  const why  = (tr.why || tr.reason || '?').toUpperCase();
  const time = tr.t   ? tr.t.substring(11,19) : '--';
  return `<div class="tl-row ${isWin ? 'win' : 'loss'}">
    <span class="tl-tag ${isWin ? 'win' : 'loss'}">${isWin ? 'WIN' : 'LOSS'}</span>
    <span class="tl-sym">${sym}</span>
    <span class="tl-pnl ${isWin ? 'pos' : 'neg'}">${fmtPnl(pnl)}</span>
    <span class="tl-pnl ${isWin ? 'pos' : 'neg'}" style="font-size:14px">${fmtUsd(usd)}</span>
    <span class="tl-eng">${tr.e || '--'}</span>
    <span class="tl-val">${en}</span>
    <span class="tl-val">${ex}</span>
    <span style="display:flex;gap:6px;align-items:center">${mfe}<span style="color:#3d5a6e">/</span>${mae}</span>
    <span style="display:flex;align-items:center;gap:8px"><span class="tl-reason ${rc}">${why}</span><span class="tl-time">${time}</span></span>
  </div>`;
}

function renderTradeLog() {
  const body = $('trade-table-body');
  if (!body) return;

  let totalPnl = 0;
  localTrades.forEach(t => totalPnl += (+t.p || 0));
  set('ts-count', localTrades.length);
  const tspnl = $('ts-pnl');
  if (tspnl) { tspnl.textContent = fmtPnl(totalPnl); tspnl.className = +totalPnl >= 0 ? 'pos' : 'neg'; }

  if (!localTrades.length) {
    body.innerHTML = '<div class="tli-empty">Waiting for first trade...</div>';
    return;
  }
  body.innerHTML = localTrades.slice(0, 80).map(makeRow).join('');
}

// ── REGIME STATE ──────────────────────────────────────────────────────────────
function regimeClass(state) {
  if (!state) return 'rs-neutral';
  const s = state.toUpperCase();
  if (s.includes('COMPRESSION')) return 'rs-compression';
  if (s.includes('BREAKOUT') || s.includes('BURST')) return 'rs-burst';
  if (s.includes('TREND') || s.includes('BUILDUP')) return 'rs-trending';
  if (s.includes('DEAD') || s.includes('GRIND')) return 'rs-dead';
  return 'rs-neutral';
}

// ── MAIN UPDATE ───────────────────────────────────────────────────────────────
let firstPoll = true;

function updateAll(data) {
  if (!data) return;

  // Uptime
  if (!uptimeStart) uptimeStart = Date.now();

  // Prices
  const btcPx = data.btc_price || 0;
  const ethPx = data.eth_price || 0;
  const solPx = data.sol_price || 0;

  const updatePrice = (id, val, prev, sym) => {
    const el = $(id); if (!el) return;
    el.textContent = fmtPrice(val, sym);
    el.className = 'sym-price' + (val > prev ? ' up' : val < prev ? ' down' : '');
  };
  updatePrice('px-btc', btcPx, lastPrices.btc, 'BTC');
  updatePrice('px-eth', ethPx, lastPrices.eth, 'ETH');
  updatePrice('px-sol', solPx, lastPrices.sol, 'SOL');
  lastPrices = { btc: btcPx, eth: ethPx, sol: solPx };

  // Top bar
  const pnl = data.pnl || 0;
  const pnlEl = $('tb-pnl');
  if (pnlEl) { pnlEl.textContent = fmtPnl(pnl); pnlEl.className = 'tb-val ' + (pnl > 0 ? 'pos' : pnl < 0 ? 'neg' : ''); }
  const equity = $('tb-equity');
  if (equity) equity.textContent = '$' + (10000 + pnl).toLocaleString('en-US', {minimumFractionDigits:2,maximumFractionDigits:2});
  set('tb-trades', data.total_trades || 0);
  set('tb-positions', data.open_positions || 0);
  const lat = data.latency_p95 || 0;
  set('tb-latency', lat > 0 ? lat.toFixed(1) + 'ms' : '--ms');

  // Session stats right panel
  set('st-pnl', fmtPnl(pnl));
  const stpnl = $('st-pnl'); if (stpnl) stpnl.className = 'sr-val ' + (pnl > 0 ? 'pos' : pnl < 0 ? 'neg' : '');
  set('st-trades', data.total_trades || 0);
  set('st-pos', data.open_positions || 0);
  set('st-lat', lat > 0 ? lat.toFixed(1) + 'ms' : '--ms');

  // Exit breakdown + by-engine from C++ session stats
  if (data.session) {
    const s = data.session;
    set('st-tp', s.tp_exits || 0);
    set('st-sl', s.sl_exits || 0);
    set('st-trail', s.trail_exits || 0);
    set('st-timeout', s.timeout_exits || 0);

    const wr = s.total_trades > 0 ? s.win_rate.toFixed(0) + '%' : '--%';
    const wrEl = $('st-wr');
    if (wrEl) { wrEl.textContent = wr; wrEl.className = 'sr-val ' + (s.wins >= s.losses ? 'pos' : 'neg'); }

    const bl = $('st-by-layer');
    if (bl && s.by_layer && s.by_layer.length) {
      bl.innerHTML = s.by_layer.map(l => `
        <div style="border-bottom:1px solid var(--border);padding:5px 0">
          <div style="display:flex;justify-content:space-between;margin-bottom:3px">
            <span style="color:var(--accent);font-family:var(--fh);font-weight:700;font-size:13px;letter-spacing:1px">${l.name}</span>
            <span style="font-weight:700;font-size:16px;color:${l.pnl>=0?'var(--green)':'var(--red)'}">${l.pnl>=0?'+':''}${(+l.pnl).toFixed(2)}bp</span>
          </div>
          <div style="display:flex;gap:10px;font-size:13px;color:var(--muted)">
            <span>${l.trades}T</span>
            <span style="color:${l.wr>=50?'var(--green)':'var(--red)'};font-weight:600">${(+l.wr).toFixed(0)}%WR</span>
            <span>avg ${l.avg_pnl>=0?'+':''}${(+l.avg_pnl).toFixed(1)}bp</span>
          </div>
          <div style="display:flex;gap:8px;margin-top:3px;font-size:10px">
            <span style="color:var(--green)">TP:${l.tp}</span>
            <span style="color:var(--red)">SL:${l.sl}</span>
            <span style="color:var(--accent)">TR:${l.trail}</span>
            <span style="color:var(--yellow)">TO:${l.timeout}</span>
            <span style="margin-left:auto;color:var(--muted)">MFE:+${(+l.avg_mfe).toFixed(1)} MAE:${(+l.avg_mae).toFixed(1)}</span>
          </div>
        </div>`).join('');
    } else if (bl) {
      bl.innerHTML = '<div style="color:var(--muted);font-size:14px">No trades yet</div>';
    }
  }

  // Per-symbol data
  [['btc','btcusdt'],['eth','ethusdt'],['sol','solusdt']].forEach(([sym, key]) => {
    const d = data[key];
    if (!d) return;

    // Regime
    const state = d.regime_state || 'NEUTRAL';
    const mult = d.regime_multiplier || 1;
    const rsEl = $(`rs-${sym}`);
    if (rsEl) { rsEl.textContent = state; rsEl.className = 'regime-state ' + regimeClass(state); }
    const rmEl = $(`rm-${sym}`);
    if (rmEl) { rmEl.textContent = '×' + mult.toFixed(2); rmEl.className = 'regime-mult ' + (mult > 1.1 ? 'hi' : mult < 0.9 ? 'lo' : ''); }

    // Sym-head meta
    set(`vr-${sym}`, d.vol_ratio ? d.vol_ratio.toFixed(2) : '--');
    set(`dp-${sym}`, d.displacement_bp != null ? d.displacement_bp.toFixed(1) + 'bp' : '--bp');
    set(`reg-${sym}`, state);
    set(`cap-${sym}`, d.dynamic_cap_R ? d.dynamic_cap_R.toFixed(2) : '--');

    // Accel / compression ticks / etc
    if (sym === 'btc') {
      set('cv-btc-vol', d.vol_ratio ? d.vol_ratio.toFixed(3) : '--');
      set('cv-btc-disp', d.displacement_bp ? d.displacement_bp.toFixed(2) + 'bp' : '--bp');
      set('cv-btc-build', (d.buildup_ticks || 0) + '/40');
      set('cv-btc-accel', d.acceleration_bp ? d.acceleration_bp.toFixed(2) + 'bp' : '--bp');
      set('cv-btc-comp', (d.compression_ticks || 0) + '/100');
      const cond = (v, t) => { const met = v >= t; return met ? 'met' : v >= t * 0.7 ? 'near' : 'off'; };
      const cc = (id, cls) => { const el = $(id); if (el) el.className = 'cond-check ' + cls; };
      cc('cc-btc-vol', cond(d.vol_ratio, 1.4));
      cc('cc-btc-disp', cond(Math.abs(d.displacement_bp||0), 20));
      cc('cc-btc-build', cond(d.buildup_ticks||0, 40));
      cc('cc-btc-accel', cond(Math.abs(d.acceleration_bp||0), 15));
      cc('cc-btc-comp', cond(d.compression_ticks||0, 100));
    }

    set(`accel-${sym}`, d.acceleration_bp != null ? d.acceleration_bp.toFixed(2) + 'bp' : '--bp');
    set(`cticks-${sym}`, (d.compression_ticks || 0) + '/100');

    // Engine cells
    const engData = [
      ['micro',       d.micro_active,        d.micro_total_pnl_bp,       d.micro_total_trades,       null,                       null],
      ['structural',  d.structural_active,   d.structural_total_pnl_bp,  d.structural_total_trades,  d.structural_win_rate,      d.structural_entry_price],
      ['convex',      d.convex_active,       d.convex_total_pnl_bp,      d.convex_total_trades,      d.convex_win_rate,          null],
      ['compression', d.compression_active,  d.compression_total_pnl_bp, d.compression_total_trades, d.compression_win_rate,     null],
      ['vacuum',      false,                 0,                           0,                          null,                       null],
      ['vwap',        false,                 0,                           0,                          null,                       null],
    ];

    engData.forEach(([eng, active, pnlBp, trades, wr, entry]) => {
      const badge = $(`eb-${sym}-${eng}`);
      if (badge) {
        badge.textContent = active ? 'ACTIVE' : 'OFF';
        badge.className = 'eng-badge ' + (active ? 'active' : 'off');
      }
      const cell = $(`ec-${sym}-${eng}`);
      if (cell) cell.className = 'eng-cell' + (active ? ' active-pos' : '');
      const sc = $(`sc-${sym}-${eng}`);
      if (sc) sc.style.background = active ? 'var(--green)' : '#0d2030';

      const pEl = $(`pnl-${sym}-${eng}`);
      if (pEl) { pEl.textContent = pnlBp != null ? fmtPnl(pnlBp) : '0bp'; pEl.className = 'est-val ' + (pnlBp > 0 ? 'pos' : pnlBp < 0 ? 'neg' : 'dim'); }
      set(`trades-${sym}-${eng}`, trades != null ? trades : 0);
      if (wr != null) {
        const wrEl = $(`wr-${sym}-${eng}`);
        if (wrEl) { wrEl.textContent = (wr * 100).toFixed(0) + '%'; wrEl.className = 'est-val ' + (wr >= 0.5 ? 'pos' : 'neg'); }
      }
      if (entry != null && entry > 0) set(`ep-${sym}-${eng}`, fmtPrice(entry, sym));
    });
  });

  // Trade log — on first poll, mark as boot load so no bell for old trades
  if (data.trade_log) mergeTrades(data.trade_log, firstPoll);
  firstPoll = false;
}

// ── POLL LOOP ─────────────────────────────────────────────────────────────────
let connected = false;

async function poll() {
  try {
    const res = await fetch('/api/state', { cache: 'no-store' });
    if (!res.ok) throw new Error('HTTP ' + res.status);
    const data = await res.json();
    if (!connected) {
      connected = true;
      const cb = $('conn-badge'); if (cb) { cb.textContent = 'LIVE'; cb.className = 'badge badge-conn'; }
      const dot = $('live-dot'); if (dot) dot.className = 'dot live';
    }
    updateAll(data);
  } catch(e) {
    if (connected) {
      connected = false;
      const cb = $('conn-badge'); if (cb) { cb.textContent = 'OFFLINE'; cb.className = 'badge badge-disc'; }
      const dot = $('live-dot'); if (dot) dot.className = 'dot';
    }
  }
}

// ── INIT ──────────────────────────────────────────────────────────────────────
loadTrades();
renderTradeLog();
updateWinRate();

// Restore wins/losses from loaded trades for accurate WR display
wins = 0; losses = 0;
localTrades.forEach(t => { if (+t.p > 0) wins++; else if (+t.p < 0) losses++; });
updateWinRate();

poll();
setInterval(poll, 1000);
setInterval(updateUptime, 1000);
