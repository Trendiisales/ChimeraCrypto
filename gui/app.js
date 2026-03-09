// ── CHIMERA app.js ────────────────────────────────────────────────────────────
// Bell fix: only fire audio/flash for trades that arrive AFTER this page load.

const STORAGE_KEY = 'chimera_trades_v3';
const BOOT_TS = Date.now();

// All symbols in display order — must match SymbolIndex.hpp
const SYMBOLS = [
  { short: 'BTC', full: 'btcusdt' },
  { short: 'ETH', full: 'ethusdt' },
  { short: 'SOL', full: 'solusdt' },
  { short: 'BNB', full: 'bnbusdt' },
  { short: 'AVAX', full: 'avaxusdt' },
  { short: 'LINK', full: 'linkusdt' },
  { short: 'POL',  full: 'polusdt'  },
];

let localTrades = [];
let audioCtx = null;
let audioUnlocked = false;
let lastPrices = {};
SYMBOLS.forEach(s => lastPrices[s.short.toLowerCase()] = 0);
let wins = 0, losses = 0;
let uptimeStart = null;

// ── AUDIO ─────────────────────────────────────────────────────────────────────
function unlockAudio() {
  if (audioUnlocked) return;
  try {
    audioCtx = new (window.AudioContext || window.webkitAudioContext)();
    const buf = audioCtx.createBuffer(1, 1, 22050);
    const src = audioCtx.createBufferSource();
    src.buffer = buf; src.connect(audioCtx.destination); src.start(0);
    audioUnlocked = true;
    const btn = document.getElementById('audio-unlock');
    if (btn) { btn.textContent = '🔔 MUTED OFF'; btn.style.background = 'rgba(0,230,118,.1)'; btn.style.color = 'var(--green)'; btn.style.borderColor = 'var(--green)'; }
  } catch(e) { console.warn('Audio unlock failed:', e); }
}

function playWin() {
  if (!audioUnlocked || !audioCtx) return;
  try {
    const now = audioCtx.currentTime;
    [[0, 1400, 1480], [0.22, 1400, 1480]].forEach(([t, f1, f2]) => {
      [f1, f2].forEach((freq, i) => {
        const osc = audioCtx.createOscillator(), gain = audioCtx.createGain(), comp = audioCtx.createDynamicsCompressor();
        comp.threshold.value = -6; comp.ratio.value = 3;
        osc.connect(gain); gain.connect(comp); comp.connect(audioCtx.destination);
        osc.type = 'sine'; osc.frequency.setValueAtTime(freq, now + t);
        gain.gain.setValueAtTime(0, now + t);
        gain.gain.linearRampToValueAtTime(i === 0 ? 1.2 : 0.6, now + t + 0.008);
        gain.gain.exponentialRampToValueAtTime(0.3, now + t + 0.08);
        gain.gain.exponentialRampToValueAtTime(0.001, now + t + 1.2);
        osc.start(now + t); osc.stop(now + t + 1.3);
      });
    });
  } catch(e) {}
}

function playLoss() {
  if (!audioUnlocked || !audioCtx) return;
  try {
    const now = audioCtx.currentTime;
    const osc = audioCtx.createOscillator(), gain = audioCtx.createGain();
    osc.connect(gain); gain.connect(audioCtx.destination);
    osc.type = 'sawtooth'; osc.frequency.setValueAtTime(220, now);
    osc.frequency.linearRampToValueAtTime(110, now + 0.2);
    gain.gain.setValueAtTime(0.2, now); gain.gain.exponentialRampToValueAtTime(0.001, now + 0.25);
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
  try { const raw = localStorage.getItem(STORAGE_KEY); localTrades = raw ? JSON.parse(raw) : []; }
  catch(e) { localTrades = []; }
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
function mergeTrades(serverLog, isBootLoad) {
  if (!serverLog || !serverLog.length) return;
  const before = localTrades.length;
  const existing = new Set(localTrades.map(t => `${t.t}|${t.s}|${t.e}|${t.p}`));
  let newCount = 0;
  serverLog.forEach(tr => {
    const key = `${tr.t}|${tr.s}|${tr.e}|${tr.p}`;
    if (!existing.has(key)) {
      localTrades.unshift(tr); existing.add(key); newCount++;
      if (!isBootLoad) {
        // Only ring bell for trades that actually just happened (within 30s)
        // Prevents bell storm when trade_log contains historical trades that
        // look "new" because localTrades was cleared (e.g. on restart detection)
        // Always treat server timestamps as UTC (append Z if no timezone suffix)
        // Without Z, browsers in non-UTC timezones (e.g. NZ +13) parse as local time
        // making every trade appear 13 hours old → bell never rings
        const tsStr = tr.t
          ? (tr.t.length < 12
              ? new Date().toISOString().slice(0,10) + 'T' + tr.t + 'Z'
              : (tr.t.endsWith('Z') || tr.t.includes('+') ? tr.t : tr.t + 'Z'))
          : null;
        const tradeAge = tsStr ? (Date.now() - new Date(tsStr).getTime()) : 99999;
        const isFresh = tradeAge < 30000;
        if (+tr.p > 0) { wins++; if (isFresh) { playWin(); flashWin(tr.s, tr.p); } }
        else if (+tr.p < 0) { losses++; if (isFresh) playLoss(); }
        else { wins++; }
      } else {
        if (+tr.p > 0) wins++; else if (+tr.p < 0) losses++;
      }
    }
  });
  if (newCount > 0 || before === 0) {
    localTrades = localTrades.slice(0, 200);
    saveTrades(); renderTradeLog(); renderSymbolTrades(); updateWinRate();
  }
}

// ── HELPERS ───────────────────────────────────────────────────────────────────
const $ = id => document.getElementById(id);
const set = (id, val) => { const el = $(id); if (el) el.textContent = val; };
const fmtPnl = v => (v >= 0 ? '+' : '') + (+v).toFixed(2) + 'bp';
const pnlCls = (base, v) => base + ' ' + (+v > 0 ? 'pos' : +v < 0 ? 'neg' : '');

const ACCOUNT_SIZE = 10000;
const bpToUsd = bp => (bp / 10000) * ACCOUNT_SIZE;
const fmtUsd = v => (v >= 0 ? '+$' : '-$') + Math.abs(v).toFixed(2);

function fmtHold(ms) {
  if (!ms || ms <= 0) return '--';
  if (ms < 1000) return ms + 'ms';
  if (ms < 60000) return (ms / 1000).toFixed(1) + 's';
  return Math.floor(ms / 60000) + 'm' + Math.floor((ms % 60000) / 1000) + 's';
}

function fmtPrice(p, sym) {
  if (!p || p <= 0) return '--';
  const s = (sym || '').toUpperCase();
  // Higher precision for lower-priced coins
  if (s === 'SOL' || s === 'LINK' || s === 'POL') return '$' + (+p).toFixed(3);
  if (s === 'AVAX') return '$' + (+p).toFixed(2);
  return '$' + (+p).toLocaleString('en-US', { minimumFractionDigits: 2, maximumFractionDigits: 2 });
}

function renderSymbolTrades() {
  SYMBOLS.forEach(({ short }) => {
    const el = $('str-' + short.toLowerCase());
    if (!el) return;
    const trades = localTrades.filter(t => (t.s || '').replace('USDT', '').toUpperCase() === short).slice(0, 8);
    if (!trades.length) { el.className = 'sym-trades-empty'; el.innerHTML = 'No trades yet'; return; }
    el.className = '';
    el.innerHTML = trades.map(tr => {
      const pnl = +tr.p || 0, isWin = pnl >= 0, usd = bpToUsd(pnl);
      const rc = reasonClass(tr.why || tr.reason || '');
      const en = tr.en ? fmtPrice(tr.en, short) : '--';
      const ex = tr.ex ? fmtPrice(tr.ex, short) : '--';
      const why = (tr.why || tr.reason || '?').toUpperCase();
      const time = tr.t ? tr.t.substring(11, 19) : '--';
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

function updateWinRate() {
  const t = wins + losses;
  const wr = t > 0 ? (wins / t * 100).toFixed(0) + '%' : '--%';
  set('ts-wr', wr);
  const tswr = $('ts-wr');
  if (tswr) tswr.className = 'ts-val ' + (t > 0 ? (wins >= losses ? 'ts-pos' : 'ts-neg') : '');
}

function updateUptime() {
  if (!uptimeStart) return;
  const s = Math.floor((Date.now() - uptimeStart) / 1000);
  const h = Math.floor(s / 3600), m = Math.floor((s % 3600) / 60), sec = s % 60;
  const fmt = `${String(h).padStart(2,'0')}:${String(m).padStart(2,'0')}:${String(sec).padStart(2,'0')}`;
  set('tb-uptime', fmt); set('st-uptime', fmt);
}

function renderTradeLog() {
  const body = $('trade-table-body');
  if (!body) return;
  let totalPnl = 0, winPnl = 0, lossPnl = 0, winCount = 0, lossCount = 0;
  localTrades.filter(t => t.s !== 'SESSION').forEach(t => {
    const p = +t.p || 0; totalPnl += p;
    if (p >= 0) { winPnl += p; winCount++; } else { lossPnl += p; lossCount++; }
  });
  const avgWin  = winCount  > 0 ? winPnl  / winCount  : null;
  const avgLoss = lossCount > 0 ? lossPnl / lossCount : null;
  const total   = winCount + lossCount;
  const wr      = total > 0 ? winCount / total : null;
  const exp     = (wr !== null && avgWin !== null && avgLoss !== null) ? (wr * avgWin + (1 - wr) * avgLoss) : null;

  set('ts-count', total); set('ts-wins', winCount); set('ts-losses', lossCount);
  const pnlEl = $('ts-pnl');
  if (pnlEl) { pnlEl.textContent = fmtPnl(totalPnl); pnlEl.className = 'ts-val ' + (totalPnl >= 0 ? 'ts-pos' : 'ts-neg'); }
  const usdEl = $('ts-usd');
  if (usdEl) { usdEl.textContent = fmtUsd(bpToUsd(totalPnl)); usdEl.className = 'ts-val ' + (totalPnl >= 0 ? 'ts-pos' : 'ts-neg'); }
  const wrEl = $('ts-wr');
  if (wrEl) { wrEl.textContent = wr !== null ? (wr*100).toFixed(0)+'%' : '--%'; wrEl.className = 'ts-val ' + (wr !== null ? (wr >= 0.5 ? 'ts-pos' : 'ts-neg') : ''); }
  const awEl = $('ts-avgwin'); if (awEl) awEl.textContent = avgWin !== null ? '+' + avgWin.toFixed(2) + 'bp' : '--';
  const alEl = $('ts-avgloss'); if (alEl) alEl.textContent = avgLoss !== null ? avgLoss.toFixed(2) + 'bp' : '--';
  const expEl = $('ts-exp');
  if (expEl) { expEl.textContent = exp !== null ? (exp >= 0 ? '+' : '') + exp.toFixed(2) + 'bp' : '--'; expEl.className = 'ts-val ' + (exp !== null ? (exp >= 0 ? 'ts-pos' : 'ts-neg') : ''); }
  if (!localTrades.length) { body.innerHTML = '<div class="tli-empty">Waiting for first trade...</div>'; return; }
  body.innerHTML = localTrades.filter(t => t.s !== 'SESSION').slice(0, 80).map(makeRow).join('');
}

// ── TRADE CARDS ───────────────────────────────────────────────────────────────
function reasonClass(r) {
  if (!r) return 'timeout';
  const rl = r.toLowerCase();
  if (rl === 'tp') return 'tp'; if (rl === 'sl') return 'sl';
  if (rl === 'trail') return 'trail'; return 'timeout';
}

function makeRow(tr) {
  const pnl = +tr.p || 0, isWin = pnl >= 0, usd = bpToUsd(pnl);
  const rc = reasonClass(tr.why || tr.reason || '');
  const sym = (tr.s || '').replace('USDT', '');
  const en = tr.en ? fmtPrice(tr.en, sym) : '--';
  const ex = tr.ex ? fmtPrice(tr.ex, sym) : '--';
  const why = (tr.why || tr.reason || '?').toUpperCase();
  const time = tr.t ? tr.t.substring(11, 19) : '--';
  return `<div class="tl-row ${isWin?'win':'loss'}">
    <span class="tl-tag ${isWin?'win':'loss'}">${isWin?'W':'L'}</span>
    <span class="tl-sym">${sym}</span>
    <span class="tl-pnl ${isWin?'pos':'neg'}">${fmtPnl(pnl)}</span>
    <span class="tl-pnl ${isWin?'pos':'neg'}">${fmtUsd(usd)}</span>
    <span class="tl-eng">${tr.e||'--'}</span>
    <span class="tl-val">${en}</span>
    <span class="tl-val">${ex}</span>
    <span class="tl-reason ${rc}">${why}</span>
    <span class="tl-time">${time}</span>
  </div>`;
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
let lastKnownUptimeHours = null;

function updateAll(data) {
  if (!data) return;
  if (!uptimeStart) uptimeStart = Date.now();

  // ── Restart detection ───────────────────────────────────────────────────
  // Server sends uptime_hours. If it goes backwards (or resets near 0),
  // the server restarted — clear stale localStorage trades from old session.
  const serverUptime = data.uptime_hours || 0;
  if (lastKnownUptimeHours !== null && serverUptime < lastKnownUptimeHours - 0.01) {
    // Server restarted — clear old session trades and silence bell on re-merge
    console.log('[Chimera] Server restart detected (uptime reset). Clearing old session trades.');
    localTrades = [];
    try { localStorage.removeItem(STORAGE_KEY); } catch(e) {}
    uptimeStart = Date.now();
    firstPoll = true;  // treat next trade merge as boot load — no bell for old trades
  }
  lastKnownUptimeHours = serverUptime;

  const updatePrice = (id, val, prev, sym) => {
    const el = $(id); if (!el) return;
    el.textContent = fmtPrice(val, sym);
    el.className = 'sym-price' + (val > prev ? ' up' : val < prev ? ' down' : '');
  };

  // Update all symbol prices dynamically
  SYMBOLS.forEach(({ short, full }) => {
    const key = short.toLowerCase();
    // Backend emits full symbol price keys: btcusdt_price, ethusdt_price etc
    const px = data[full + '_price'] || data[short.toUpperCase() + '_price'] || data[short.toLowerCase() + '_price'] || 0;
    updatePrice('px-' + key, px, lastPrices[key], short);
    lastPrices[key] = px;
  });

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

  set('st-pnl', fmtPnl(pnl));
  const stpnl = $('st-pnl'); if (stpnl) stpnl.className = 'sr-val ' + (pnl > 0 ? 'pos' : pnl < 0 ? 'neg' : '');
  set('st-trades', data.total_trades || 0);

  // Override ts-pnl with server-authoritative value (localStorage can have stale trades from old sessions)
  const tsPnlEl = $('ts-pnl');
  if (tsPnlEl && data.pnl !== undefined) {
    tsPnlEl.textContent = fmtPnl(pnl);
    tsPnlEl.className = 'ts-val ' + (pnl >= 0 ? 'ts-pos' : 'ts-neg');
  }
  const tsCountEl = $('ts-count');
  if (tsCountEl && data.total_trades !== undefined) tsCountEl.textContent = data.total_trades || 0;
  set('st-pos', data.open_positions || 0);
  set('st-lat', lat > 0 ? lat.toFixed(1) + 'ms' : '--ms');

  // Exit breakdown
  if (data.session) {
    const s = data.session;
    set('st-tp', s.tp_exits || 0); set('st-sl', s.sl_exits || 0);
    set('st-trail', s.trail_exits || 0); set('st-timeout', s.timeout_exits || 0);
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

  // Per-symbol blocks — dynamic over all SYMBOLS
  SYMBOLS.forEach(({ short, full }) => {
    const sym = short.toLowerCase();
    const d = data[full];
    if (!d) return;

    // Regime
    const state = d.regime_state || 'NEUTRAL';
    const mult = d.regime_multiplier || 1;
    const rsEl = $(`rs-${sym}`);
    if (rsEl) { rsEl.textContent = state; rsEl.className = 'regime-state ' + regimeClass(state); }
    const rmEl = $(`rm-${sym}`);
    if (rmEl) { rmEl.textContent = '×' + mult.toFixed(2); rmEl.className = 'regime-mult ' + (mult > 1.1 ? 'hi' : mult < 0.9 ? 'lo' : ''); }

    // Mini summary (shown when block is collapsed)
    const miniPnl = $(`mini-pnl-${sym}`);
    const microPnl = d.micro_total_pnl_bp || 0;
    const microT   = d.micro_total_trades || 0;
    if (miniPnl) { miniPnl.textContent = (microPnl >= 0 ? '+' : '') + microPnl.toFixed(2) + 'bp'; miniPnl.className = 'sm-pnl ' + (microPnl > 0 ? 'pos' : microPnl < 0 ? 'neg' : ''); }
    set(`mini-t-${sym}`, microT);
    // Auto-expand if micro engine goes active
    autoExpandIfActive(sym, d.micro_active);

    set(`vr-${sym}`,  d.vol_ratio      ? d.vol_ratio.toFixed(2)       : '--');
    set(`dp-${sym}`,  d.displacement_bp != null ? d.displacement_bp.toFixed(1) + 'bp' : '--bp');
    set(`reg-${sym}`, state);
    set(`cap-${sym}`, d.dynamic_cap_R  ? d.dynamic_cap_R.toFixed(2)   : '--');

    // BTC signal conditions panel (BTC only)
    if (sym === 'btc') {
      set('cv-btc-vol',  d.vol_ratio      ? d.vol_ratio.toFixed(3)      : '--');
      set('cv-btc-disp', d.displacement_bp ? d.displacement_bp.toFixed(2) + 'bp' : '--bp');
      set('cv-btc-build', (d.buildup_ticks || 0) + '/40');
      set('cv-btc-accel', d.acceleration_bp ? d.acceleration_bp.toFixed(2) + 'bp' : '--bp');
      set('cv-btc-comp', (d.compression_ticks || 0) + '/100');
      const cond = (v, t) => v >= t ? 'met' : v >= t * 0.7 ? 'near' : 'off';
      const cc = (id, cls) => { const el = $(id); if (el) el.className = 'cond-check ' + cls; };
      cc('cc-btc-vol',   cond(d.vol_ratio || 0, 1.4));
      cc('cc-btc-disp',  cond(Math.abs(d.displacement_bp || 0), 20));
      cc('cc-btc-build', cond(d.buildup_ticks || 0, 40));
      cc('cc-btc-accel', cond(Math.abs(d.acceleration_bp || 0), 15));
      cc('cc-btc-comp',  cond(d.compression_ticks || 0, 100));
    }

    set(`accel-${sym}`,  d.acceleration_bp  != null ? d.acceleration_bp.toFixed(2) + 'bp' : '--bp');
    set(`cticks-${sym}`, (d.compression_ticks || 0) + '/100');

    const engData = [
      ['micro',       d.micro_active,       d.micro_total_pnl_bp,       d.micro_total_trades,       null,               null],
      ['structural',  d.structural_active,  d.structural_total_pnl_bp,  d.structural_total_trades,  d.structural_win_rate, d.structural_entry_price],
      ['convex',      d.convex_active,      d.convex_total_pnl_bp,      d.convex_total_trades,      d.convex_win_rate,  null],
      ['compression', d.compression_active, d.compression_total_pnl_bp, d.compression_total_trades, d.compression_win_rate, null],
      ['vacuum',      false, 0, 0, null, null],
      ['vwap',        false, 0, 0, null, null],
    ];
    engData.forEach(([eng, active, pnlBp, trades, wr, entry]) => {
      const badge = $(`eb-${sym}-${eng}`);
      if (badge) { badge.textContent = active ? 'ACTIVE' : 'OFF'; badge.className = 'eng-badge ' + (active ? 'active' : 'off'); }
      const cell = $(`ec-${sym}-${eng}`);
      if (cell) cell.className = 'eng-cell' + (active ? ' active-pos' : '');
      const sc = $(`sc-${sym}-${eng}`);
      if (sc) sc.style.background = active ? 'var(--green)' : '#0d2030';
      const pEl = $(`pnl-${sym}-${eng}`);
      if (pEl) { pEl.textContent = pnlBp != null ? fmtPnl(pnlBp) : '0bp'; pEl.className = 'est-val ' + (pnlBp > 0 ? 'pos' : pnlBp < 0 ? 'neg' : 'dim'); }
      set(`trades-${sym}-${eng}`, trades != null ? trades : 0);
      if (wr != null) { const wrEl = $(`wr-${sym}-${eng}`); if (wrEl) { wrEl.textContent = (wr * 100).toFixed(0) + '%'; wrEl.className = 'est-val ' + (wr >= 0.5 ? 'pos' : 'neg'); } }
      if (entry != null && entry > 0) set(`ep-${sym}-${eng}`, fmtPrice(entry, sym));
    });
  });

  if (data.trade_log) mergeTrades(data.trade_log, firstPoll);
  firstPoll = false;
}

// ── POLL LOOP ─────────────────────────────────────────────────────────────────
let connected = false;
let pollErrors = 0;
let lastPollOk = null;
let lastPollErrorMsg = '';

function fmtAgo(ts) {
  if (!ts) return 'never';
  const s = Math.floor((Date.now() - ts) / 1000);
  if (s < 60) return s + 's ago';
  return Math.floor(s/60) + 'm' + (s%60) + 's ago';
}

function setPollOk() {
  pollErrors = 0;
  lastPollOk = Date.now();
  const cb = $('conn-badge');
  if (cb) { cb.textContent = 'LIVE'; cb.className = 'badge badge-conn'; }
  const dot = $('live-dot');
  if (dot) dot.className = 'dot live';
  const banner = $('poll-error');
  if (banner) banner.classList.remove('show');
  const lp = $('tb-last-poll');
  if (lp) { lp.textContent = 'now'; lp.className = 'tb-val'; }
}

function setPollError(reason) {
  pollErrors++;
  lastPollErrorMsg = reason;
  connected = false;

  // Badge
  const cb = $('conn-badge');
  if (cb) { cb.textContent = 'OFFLINE'; cb.className = 'badge badge-disc'; }
  const dot = $('live-dot');
  if (dot) dot.className = 'dot';

  // Error banner — always visible with full detail
  const banner = $('poll-error');
  if (banner) banner.classList.add('show');
  const msg = $('poll-error-msg');
  if (msg) msg.textContent = reason;
  const cnt = $('poll-error-count');
  if (cnt) cnt.textContent = 'ERR×' + pollErrors;
  const etime = $('poll-error-time');
  if (etime) etime.textContent = 'last ok: ' + fmtAgo(lastPollOk);

  // Last-poll indicator — goes red when stale
  const lp = $('tb-last-poll');
  if (lp) {
    lp.textContent = fmtAgo(lastPollOk);
    lp.className = 'tb-val stale';
  }
}

async function poll() {
  let res;
  try {
    res = await fetch('/api/state', { cache: 'no-store', signal: AbortSignal.timeout(4000) });
  } catch(e) {
    // Network-level failure: timeout, DNS, connection refused etc
    const reason = e.name === 'TimeoutError' ? 'Fetch timeout (>4s) — backend hung?' :
                   e.name === 'TypeError'    ? 'Network error — backend down or unreachable' :
                   'Fetch failed: ' + e.message;
    setPollError(reason);
    return;
  }

  if (!res.ok) {
    setPollError('HTTP ' + res.status + ' ' + res.statusText + ' — backend returned error');
    return;
  }

  let data;
  try {
    data = await res.json();
  } catch(e) {
    // Got a response but it wasn't valid JSON — backend probably crashed mid-write
    let raw = '';
    try { raw = await res.text(); } catch(_) {}
    setPollError('JSON parse error — backend may have crashed. Preview: ' + raw.slice(0, 80));
    return;
  }

  // All good
  if (!connected) connected = true;
  setPollOk();
  updateAll(data);
}

// Update last-poll age every second so it counts up correctly when offline
setInterval(() => {
  if (pollErrors > 0) {
    const etime = $('poll-error-time');
    if (etime) etime.textContent = 'last ok: ' + fmtAgo(lastPollOk);
    const lp = $('tb-last-poll');
    if (lp) lp.textContent = fmtAgo(lastPollOk);
  }
}, 1000);

// ── INIT ──────────────────────────────────────────────────────────────────────
loadTrades(); renderTradeLog(); updateWinRate();
wins = 0; losses = 0;
localTrades.filter(t => t.s !== 'SESSION').forEach(t => { if (+t.p > 0) wins++; else if (+t.p < 0) losses++; });
updateWinRate();
poll();
setInterval(poll, 1000);
setInterval(updateUptime, 1000);

// ── COLLAPSIBLE SYM-BLOCKS ────────────────────────────────────────────────────
function toggleBlock(sl, event) {
  // Only toggle if click was on the header itself, not on trade rows or eng-cells
  if (event) event.stopPropagation();
  const block = document.getElementById('sb-' + sl);
  if (!block) return;
  block.classList.toggle('collapsed');
}

// Auto-expand a block when it has an active trade
function autoExpandIfActive(sl, isActive) {
  const block = document.getElementById('sb-' + sl);
  if (!block) return;
  if (isActive && block.classList.contains('collapsed')) {
    block.classList.remove('collapsed');
  }
}
