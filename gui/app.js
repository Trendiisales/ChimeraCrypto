(function () {
'use strict';

const $ = id => document.getElementById(id);
const fmt = (v, d = 2) => (v == null || isNaN(+v)) ? '--' : (+v).toFixed(d);
const fmtPx = (v, decimals) => v > 0
  ? '$' + (+v).toLocaleString('en-US', { minimumFractionDigits: decimals, maximumFractionDigits: decimals })
  : '--';

// ── AUDIO ─────────────────────────────────────────────────────────────────
let _ctx = null;
let _audioUnlocked = false;

function unlockAudio() {
  if (_audioUnlocked) return;
  try {
    _ctx = new (window.AudioContext || window.webkitAudioContext)();
    // Play a silent buffer to unlock
    const buf = _ctx.createBuffer(1, 1, 22050);
    const src = _ctx.createBufferSource();
    src.buffer = buf; src.connect(_ctx.destination); src.start(0);
    _audioUnlocked = true;
    const btn = document.getElementById('audio-unlock');
    if (btn) { btn.style.display = 'none'; }
  } catch(e) {}
}

// Unlock on any interaction
document.addEventListener('click', unlockAudio, { once: true });
document.addEventListener('keydown', unlockAudio, { once: true });

function playWin() {
  try {
    if (!_ctx) _ctx = new (window.AudioContext || window.webkitAudioContext)();
    const tone = (f, t, d, g) => {
      const o = _ctx.createOscillator(), gn = _ctx.createGain();
      o.connect(gn); gn.connect(_ctx.destination); o.type = 'sine';
      o.frequency.setValueAtTime(f, t);
      gn.gain.setValueAtTime(0, t);
      gn.gain.linearRampToValueAtTime(g, t + 0.01);
      gn.gain.exponentialRampToValueAtTime(0.001, t + d);
      o.start(t); o.stop(t + d);
    };
    const t = _ctx.currentTime;
    tone(880, t, 0.6, 0.4); tone(1108, t + 0.05, 0.5, 0.25); tone(659, t + 0.15, 0.8, 0.2);
  } catch (e) {}
}

// ── WIN FLASH ─────────────────────────────────────────────────────────────
function flashWin(sym, pnl) {
  const el = $('win-flash'); if (!el) return;
  el.textContent = `🔔 WIN  ${sym.replace('usdt','').replace('USDT','')}  +${(+pnl).toFixed(2)}bp`;
  el.style.display = 'block'; el.classList.add('show');
  setTimeout(() => { el.classList.remove('show'); el.style.display = 'none'; }, 2800);
}

// ── UPTIME ────────────────────────────────────────────────────────────────
const t0 = Date.now();
setInterval(() => {
  const s = Math.floor((Date.now() - t0) / 1000);
  const str = [Math.floor(s / 3600), Math.floor((s % 3600) / 60), s % 60]
    .map(n => String(n).padStart(2, '0')).join(':');
  ['tb-uptime', 'st-uptime'].forEach(id => { const e = $(id); if (e) e.textContent = str; });
}, 1000);

// ── SIGNAL COLUMN ANIMATION ───────────────────────────────────────────────
// Each engine has an animated vertical "charge" column
// that rises as readiness approaches 1.0
const signalColumns = {};

function initSignalColumn(id) {
  const el = $(id); if (!el || signalColumns[id]) return;
  signalColumns[id] = { el, current: 0, target: 0, raf: null };
}

function updateSignalColumn(id, readiness) {
  if (!signalColumns[id]) initSignalColumn(id);
  const sc = signalColumns[id]; if (!sc) return;
  sc.target = Math.max(0, Math.min(1, readiness));
}

// Smooth animation loop for all columns
function animateColumns() {
  requestAnimationFrame(animateColumns);
  const now = Date.now();
  Object.values(signalColumns).forEach(sc => {
    const diff = sc.target - sc.current;
    sc.current += diff * 0.06; // smooth interpolation
    const pct = sc.current * 100;
    const el = sc.el; if (!el) return;

    // Color gradient: dark blue → cyan → yellow → orange → green
    let color, glow;
    if (pct < 30)      { color = '#1a3a5c'; glow = 'none'; }
    else if (pct < 55) { color = `linear-gradient(to top, #00e5ff ${pct}%, #0d2030 ${pct}%)`; glow = '0 0 6px rgba(0,229,255,0.3)'; }
    else if (pct < 75) { color = `linear-gradient(to top, #ffd600 ${pct}%, #1a1200 ${pct}%)`; glow = '0 0 10px rgba(255,214,0,0.4)'; }
    else if (pct < 90) { color = `linear-gradient(to top, #ff6d00 ${pct}%, #1a0800 ${pct}%)`; glow = '0 0 14px rgba(255,109,0,0.5)'; }
    else               { color = `linear-gradient(to top, #00e676 ${pct}%, #001a09 ${pct}%)`; glow = '0 0 18px rgba(0,230,118,0.7)'; }

    el.style.background = color;
    el.style.boxShadow = glow;

    // Pulse when near-ready (>85%)
    if (pct > 85) {
      const pulse = 0.85 + 0.15 * Math.sin(now / 200);
      el.style.opacity = String(pulse);
    } else {
      el.style.opacity = '1';
    }
  });
}
animateColumns();

// ── BADGE HELPERS ─────────────────────────────────────────────────────────
function setEngBadge(id, active, armed) {
  const el = $(id); if (!el) return;
  if (active)     { el.textContent = 'ACTIVE'; el.className = 'eng-badge active'; }
  else if (armed) { el.textContent = 'ARMED';  el.className = 'eng-badge armed'; }
  else            { el.textContent = 'OFF';    el.className = 'eng-badge off'; }
  const cell = el.closest('.eng-cell');
  if (cell) cell.classList.toggle('active-pos', !!active);
}

function setPnl(id, val) {
  const el = $(id); if (!el) return;
  const v = +val || 0;
  el.textContent = (v >= 0 ? '+' : '') + v.toFixed(2) + 'bp';
  el.className = 'est-val ' + (v > 0 ? 'pos' : v < 0 ? 'neg' : 'dim');
}

function setCond(id, met, near) {
  const el = $(id); if (!el) return;
  el.className = 'cond-check ' + (met ? 'met' : near ? 'near' : 'off');
}

// ── REGIME ────────────────────────────────────────────────────────────────
const REGIME_CLASS = {
  NEUTRAL: 'rs-neutral', GRIND: 'rs-neutral',
  TRENDING: 'rs-trending', EXPANSION: 'rs-trending',
  BURST: 'rs-burst', BREAKOUT: 'rs-burst',
  COMPRESSION: 'rs-compression', DEAD: 'rs-dead'
};

function setRegime(s, state, mult) {
  const rs = $(`rs-${s}`), rm = $(`rm-${s}`);
  if (rs) { rs.textContent = state || 'NEUTRAL'; rs.className = 'regime-state ' + (REGIME_CLASS[state] || 'rs-neutral'); }
  if (rm) { const m = +mult || 1; rm.textContent = '×' + m.toFixed(2); rm.className = 'regime-mult' + (m >= 1.4 ? ' hi' : m <= 0.6 ? ' lo' : ''); }
}

// ── PRICE ─────────────────────────────────────────────────────────────────
const lastPx = {};
function setPrice(sym, val, dec) {
  const el = $(`px-${sym}`); if (!el || !val || val <= 0) return;
  const prev = lastPx[sym] || val;
  el.textContent = fmtPx(val, dec);
  el.className = 'sym-price ' + (val > prev ? 'up' : val < prev ? 'down' : '');
  lastPx[sym] = val;
}

// ── TRADE LOG ─────────────────────────────────────────────────────────────
const STORAGE_KEY = 'chimera_trades_v3';
let localTrades = [];
let wins = 0, losses = 0;

// Load persisted trades on boot
(function loadPersistedTrades() {
  try {
    const raw = localStorage.getItem(STORAGE_KEY);
    if (raw) {
      localTrades = JSON.parse(raw) || [];
      wins = localTrades.filter(t => +t.p > 0).length;
      losses = localTrades.filter(t => +t.p < 0).length;
    }
  } catch(e) { localTrades = []; }
})();

function saveTrades() {
  try { localStorage.setItem(STORAGE_KEY, JSON.stringify(localTrades.slice(0, 200))); } catch(e) {}
}

function mergeTrades(serverLog) {
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
      if (+tr.p > 0) { wins++; playWin(); flashWin(tr.s, tr.p); }
      else if (+tr.p < 0) losses++;
    }
  });
  if (newCount > 0 || before === 0) {
    localTrades = localTrades.slice(0, 200);
    saveTrades();
    renderTradeLog();
    updateWinRate();
  }
}

function fmtHold(ms) {
  if (!ms || ms <= 0) return '--';
  if (ms < 1000) return ms + 'ms';
  if (ms < 60000) return (ms / 1000).toFixed(1) + 's';
  return (ms / 60000).toFixed(1) + 'm';
}

function fmtPxCompact(v) {
  if (!v || v <= 0) return '--';
  const n = +v;
  if (n > 10000) return '$' + n.toLocaleString('en-US', {maximumFractionDigits: 0});
  if (n > 100) return '$' + n.toFixed(2);
  return '$' + n.toFixed(3);
}

function renderTradeLog() {
  const list = $('trade-card-list'); if (!list) return;
  if (!localTrades.length) {
    list.innerHTML = '<div style="color:var(--muted);font-size:10px;display:flex;align-items:center;padding:0 10px">Waiting for first trade...</div>';
    return;
  }
  const shown = localTrades.slice(0, 80);
  list.innerHTML = shown.map(tr => {
    const p = +tr.p, pos = p >= 0;
    const pnlStr = (pos ? '+' : '') + p.toFixed(2) + 'bp';
    const sym = (tr.s || '').replace('usdt','').replace('USDT','').toUpperCase();
    const mfe = tr.mfe != null ? (+tr.mfe).toFixed(2) : '--';
    const mae = tr.mae != null ? (+tr.mae).toFixed(2) : '--';
    const hold = fmtHold(tr.hold);
    const en = fmtPxCompact(tr.en);
    const ex = fmtPxCompact(tr.ex);
    const why = (tr.why || (pos ? 'TP' : 'SL')).toUpperCase();
    const whyCls = why === 'TP' ? 'tp' : why === 'SL' ? 'sl' : why === 'TRAIL' ? 'trail' : 'timeout';
    return `<div class="trade-card ${pos ? 'win' : 'loss'}">
      <div class="tc-header">
        <span class="tc-sym">${sym}</span>
        <span class="tc-pnl ${pos ? 'pos' : 'neg'}">${pnlStr}${pos ? ' 🔔' : ''}</span>
      </div>
      <div class="tc-row">
        <span class="tc-lbl">${tr.e || '--'}</span>
        <span class="tc-reason ${whyCls}">${why}</span>
      </div>
      <div class="tc-row">
        <span class="tc-lbl">Entry</span><span class="tc-v">${en}</span>
        <span class="tc-lbl" style="margin-left:6px">Exit</span><span class="tc-v">${ex}</span>
      </div>
      <div class="tc-row">
        <span class="tc-lbl">MFE</span><span class="tc-v pos">+${mfe}bp</span>
        <span class="tc-lbl" style="margin-left:6px">MAE</span><span class="tc-v neg">${mae}bp</span>
      </div>
      <div class="tc-row">
        <span class="tc-lbl">Hold</span><span class="tc-v dim">${hold}</span>
        <span class="tc-lbl" style="margin-left:6px">${tr.t || '--'}</span>
      </div>
    </div>`;
  }).join('');

  // Update strip header stats
  const total = wins + losses;
  const wr = total > 0 ? (wins / total * 100).toFixed(0) + '%' : '--%';
  const tc = $('ts-count'); if (tc) tc.textContent = total;
  const tw = $('ts-wr'); if (tw) { tw.textContent = wr; tw.className = wins >= losses ? 'strong pos' : 'strong neg'; }
  const cumPnl = localTrades.reduce((a, t) => a + (+t.p || 0), 0);
  const tp = $('ts-pnl');
  if (tp) { tp.textContent = (cumPnl >= 0 ? '+' : '') + cumPnl.toFixed(2) + 'bp'; tp.className = cumPnl >= 0 ? 'pos' : 'neg'; }
  const tw2 = $('ts-pnl-wrap'); if (tw2) tw2.className = 'ts-stat ' + (cumPnl >= 0 ? 'pos' : 'neg');
}

function updateWinRate() {
  const total = wins + losses;
  const wr = total > 0 ? (wins / total * 100).toFixed(0) + '%' : '--%';
  const el = $('st-wr');
  if (el) { el.textContent = wr; el.className = 'sr-val ' + (wins >= losses ? 'pos' : 'neg'); }
}

window.clearTrades = function() {
  localTrades = []; wins = 0; losses = 0;
  try { localStorage.removeItem(STORAGE_KEY); } catch(e) {}
  renderTradeLog();
  updateWinRate();
};

// ── MAIN UPDATE ───────────────────────────────────────────────────────────
function updateAll(data) {
  if (!data) return;

  // Top bar
  const pnl = +data.pnl || 0;
  const equity = 10000 + pnl;
  const el_eq = $('tb-equity');
  if (el_eq) el_eq.textContent = '$' + equity.toLocaleString('en-US', { minimumFractionDigits: 2, maximumFractionDigits: 2 });

  const el_pnl = $('tb-pnl');
  if (el_pnl) { el_pnl.textContent = (pnl >= 0 ? '+' : '') + pnl.toFixed(2) + 'bp'; el_pnl.className = 'tb-val ' + (pnl > 0 ? 'pos' : pnl < 0 ? 'neg' : ''); }

  const el_tr = $('tb-trades'); if (el_tr) el_tr.textContent = data.total_trades || 0;
  const el_pos = $('tb-positions'); if (el_pos) el_pos.textContent = data.open_positions || 0;

  const lat = +data.latency_p95 || 0;
  const el_lat = $('tb-latency');
  if (el_lat) { el_lat.textContent = lat > 0 ? lat.toFixed(1) + 'ms' : '--ms'; el_lat.className = 'tb-val ' + (lat > 0 && lat < 25 ? 'pos' : lat > 50 ? 'neg' : 'accent'); }

  // Stats panel
  const sp = $('st-pnl'); if (sp) { sp.textContent = (pnl >= 0 ? '+' : '') + pnl.toFixed(2) + 'bp'; sp.className = 'sr-val ' + (pnl > 0 ? 'pos' : pnl < 0 ? 'neg' : ''); }
  const st = $('st-trades'); if (st) st.textContent = data.total_trades || 0;
  const spos = $('st-pos'); if (spos) spos.textContent = data.open_positions || 0;
  const slat = $('st-lat'); if (slat) slat.textContent = lat > 0 ? lat.toFixed(1) + 'ms' : '--ms';

  // Exit breakdown from session stats
  if (data.session) {
    const s = data.session;
    const tp = $('st-tp'); if (tp) tp.textContent = s.tp_exits || 0;
    const sl = $('st-sl'); if (sl) sl.textContent = s.sl_exits || 0;
    const tr = $('st-trail'); if (tr) tr.textContent = s.trail_exits || 0;
    const to = $('st-timeout'); if (to) to.textContent = s.timeout_exits || 0;

    // Win rate from live C++ stats (authoritative)
    const wr = s.total_trades > 0 ? s.win_rate.toFixed(0) + '%' : '--%';
    const el = $('st-wr');
    if (el) { el.textContent = wr; el.className = 'sr-val ' + (s.wins >= s.losses ? 'pos' : 'neg'); }

    // Per-layer breakdown table
    const bl = $('st-by-layer');
    if (bl && s.by_layer && s.by_layer.length) {
      bl.innerHTML = s.by_layer.map(l => `
        <div style="border-bottom:1px solid var(--border);padding:4px 0;font-size:9px">
          <div style="display:flex;justify-content:space-between;margin-bottom:2px">
            <span style="color:var(--accent);font-weight:700;letter-spacing:1px">${l.name}</span>
            <span style="font-weight:700;color:${l.pnl>=0?'var(--green)':'var(--red)'}">${l.pnl>=0?'+':''}${(+l.pnl).toFixed(2)}bp</span>
          </div>
          <div style="display:flex;gap:8px;color:var(--muted)">
            <span>${l.trades}T</span>
            <span style="color:${l.wr>=50?'var(--green)':'var(--red)'}">${l.wr.toFixed(0)}%WR</span>
            <span>avg ${l.avg_pnl>=0?'+':''}${(+l.avg_pnl).toFixed(1)}bp</span>
          </div>
          <div style="display:flex;gap:6px;margin-top:2px;font-size:8px">
            <span style="color:var(--green)">TP:${l.tp}</span>
            <span style="color:var(--red)">SL:${l.sl}</span>
            <span style="color:var(--accent)">TR:${l.trail}</span>
            <span style="color:var(--yellow)">TO:${l.timeout}</span>
            <span style="color:var(--muted);margin-left:auto">MFE:+${(+l.avg_mfe).toFixed(1)} MAE:${(+l.avg_mae).toFixed(1)}</span>
          </div>
        </div>`).join('');
    } else if (bl) {
      bl.innerHTML = '<div style="color:var(--muted);font-size:9px">No trades yet</div>';
    }
  }

  // Trade log from server
  if (data.trade_log) mergeTrades(data.trade_log);

  // Prices
  if (data.btc_price > 0) setPrice('btc', data.btc_price, 2);
  if (data.eth_price > 0) setPrice('eth', data.eth_price, 2);
  if (data.sol_price > 0) setPrice('sol', data.sol_price, 3);

  const SYMS = ['btcusdt', 'ethusdt', 'solusdt'];
  SYMS.forEach(sym => {
    const s = sym.slice(0, 3);
    const d = data[sym]; if (!d) return;

    // Header micro data
    const vr = +d.vol_ratio || 1;
    const dp = +d.displacement_bp || 0;
    const vr_el = $(`vr-${s}`);
    if (vr_el) { vr_el.textContent = vr.toFixed(3); vr_el.style.color = vr > 1.4 ? 'var(--green)' : vr > 1.2 ? 'var(--yellow)' : 'var(--text)'; }
    const dp_el = $(`dp-${s}`);
    if (dp_el) { dp_el.textContent = (dp >= 0 ? '+' : '') + dp.toFixed(1) + 'bp'; dp_el.style.color = Math.abs(dp) > 20 ? 'var(--yellow)' : 'var(--text)'; }
    const cap_el = $(`cap-${s}`); if (cap_el) cap_el.textContent = (+d.dynamic_cap_R || 0).toFixed(2);
    const reg_el = $(`reg-${s}`); if (reg_el) reg_el.textContent = d.regime_state || '--';

    setRegime(s, d.regime_state, d.regime_multiplier);

    // MICRO
    setEngBadge(`eb-${s}-micro`, d.micro_active, false);
    setPnl(`pnl-${s}-micro`, d.micro_total_pnl_bp);
    const tr_m = $(`trades-${s}-micro`); if (tr_m) tr_m.textContent = d.micro_total_trades || 0;

    // STRUCTURAL
    const struct_ready = +d.structural_readiness || 0;
    setEngBadge(`eb-${s}-structural`, d.structural_active, struct_ready > 0.7);
    setPnl(`pnl-${s}-structural`, d.structural_total_pnl_bp);
    const tr_st = $(`trades-${s}-structural`); if (tr_st) tr_st.textContent = d.structural_total_trades || 0;
    const wr_st = $(`wr-${s}-structural`); if (wr_st) wr_st.textContent = d.structural_win_rate > 0 ? (d.structural_win_rate * 100).toFixed(0) + '%' : '--%';
    const ep_st = $(`ep-${s}-structural`); if (ep_st) ep_st.textContent = d.structural_entry_price > 0 ? fmtPx(d.structural_entry_price, 2) : '--';
    updateSignalColumn(`sc-${s}-structural`, struct_ready);

    // CONVEX
    const convex_ready = +d.convex_readiness || 0;
    setEngBadge(`eb-${s}-convex`, d.convex_active, convex_ready > 0.7);
    setPnl(`pnl-${s}-convex`, d.convex_total_pnl_bp);
    const tr_cv = $(`trades-${s}-convex`); if (tr_cv) tr_cv.textContent = d.convex_total_trades || 0;
    const wr_cv = $(`wr-${s}-convex`); if (wr_cv) wr_cv.textContent = d.convex_win_rate > 0 ? (d.convex_win_rate * 100).toFixed(0) + '%' : '--%';
    const accel_el = $(`accel-${s}`);
    if (accel_el) { const a = +d.acceleration_bp || 0; accel_el.textContent = (a >= 0 ? '+' : '') + a.toFixed(1) + 'bp'; accel_el.className = 'est-val ' + (Math.abs(a) >= 15 ? 'pos' : 'dim'); }
    updateSignalColumn(`sc-${s}-convex`, convex_ready);

    // COMPRESSION
    const comp_ready = +d.compression_readiness || 0;
    const cticks = +d.compression_ticks || 0;
    setEngBadge(`eb-${s}-compression`, d.compression_active, cticks >= 80);
    setPnl(`pnl-${s}-compression`, d.compression_total_pnl_bp);
    const tr_cp = $(`trades-${s}-compression`); if (tr_cp) tr_cp.textContent = d.compression_total_trades || 0;
    const wr_cp = $(`wr-${s}-compression`); if (wr_cp) wr_cp.textContent = d.compression_win_rate > 0 ? (d.compression_win_rate * 100).toFixed(0) + '%' : '--%';
    const ct_el = $(`cticks-${s}`); if (ct_el) ct_el.textContent = cticks + '/100';
    updateSignalColumn(`sc-${s}-compression`, comp_ready);

    // VACUUM
    const vac_active = !!d.vacuum_active;
    const vac_drain = +d.vacuum_ask_drain_pct || 0;
    setEngBadge(`eb-${s}-vacuum`, vac_active, vac_drain >= 30);
    setPnl(`pnl-${s}-vacuum`, d.vacuum_total_pnl_bp);
    const tr_vac = $(`trades-${s}-vacuum`); if (tr_vac) tr_vac.textContent = d.vacuum_total_trades || 0;
    const wr_vac = $(`wr-${s}-vacuum`); if (wr_vac) wr_vac.textContent = d.vacuum_win_rate > 0 ? (d.vacuum_win_rate * 100).toFixed(0) + '%' : '--%';
    const drain_el = $(`drain-${s}`);
    if (drain_el) { drain_el.textContent = vac_drain.toFixed(0) + '%'; drain_el.className = 'est-val ' + (vac_drain >= 40 ? 'pos' : 'dim'); }
    updateSignalColumn(`sc-${s}-vacuum`, vac_drain / 100);

    // VWAP REVERSION
    const vwap_active = !!d.vwap_active;
    const vwap_dev = +d.vwap_deviation_bp || 0;
    setEngBadge(`eb-${s}-vwap`, vwap_active, vwap_dev >= 20);
    setPnl(`pnl-${s}-vwap`, d.vwap_total_pnl_bp);
    const tr_vw = $(`trades-${s}-vwap`); if (tr_vw) tr_vw.textContent = d.vwap_total_trades || 0;
    const wr_vw = $(`wr-${s}-vwap`); if (wr_vw) wr_vw.textContent = d.vwap_win_rate > 0 ? (d.vwap_win_rate * 100).toFixed(0) + '%' : '--%';
    const dev_el = $(`vwapdev-${s}`);
    if (dev_el) { dev_el.textContent = (vwap_dev >= 0 ? '-' : '+') + Math.abs(vwap_dev).toFixed(1) + 'bp'; dev_el.className = 'est-val ' + (vwap_dev >= 20 ? 'accent' : 'dim'); }
    updateSignalColumn(`sc-${s}-vwap`, Math.min(1, vwap_dev / 40));
  });

  // BTC signal condition panel
  const bd = data.btcusdt;
  if (bd) {
    const vol = +bd.vol_ratio || 1, disp = Math.abs(+bd.displacement_bp || 0);
    const build = +bd.buildup_ticks || 0, accel = Math.abs(+bd.acceleration_bp || 0);
    const comp = +bd.compression_ticks || 0;

    const cv = (id, txt) => { const e = $(id); if (e) e.textContent = txt; };
    cv('cv-btc-vol',   vol.toFixed(3) + ' / 1.4');   setCond('cc-btc-vol',   vol >= 1.4, vol >= 1.2);
    cv('cv-btc-disp',  disp.toFixed(1) + ' / 20bp'); setCond('cc-btc-disp',  disp >= 20, disp >= 12);
    cv('cv-btc-build', build + ' / 40');              setCond('cc-btc-build', build >= 40, build >= 25);
    cv('cv-btc-accel', accel.toFixed(1) + ' / 15bp');setCond('cc-btc-accel', accel >= 15, accel >= 8);
    cv('cv-btc-comp',  comp + ' / 100');              setCond('cc-btc-comp',  comp >= 100, comp >= 70);
  }
}

// ── HTTP POLL ─────────────────────────────────────────────────────────────
let fails = 0;
function poll() {
  fetch('/api/state')
    .then(r => r.json())
    .then(data => {
      fails = 0;
      updateAll(data);
      const cb = $('conn-badge'); if (cb) { cb.textContent = 'LIVE'; cb.className = 'badge badge-conn'; }
      const dot = $('live-dot'); if (dot) dot.className = 'dot live';
    })
    .catch(() => {
      if (++fails > 3) {
        const cb = $('conn-badge'); if (cb) { cb.textContent = 'OFFLINE'; cb.className = 'badge badge-disc'; }
        const dot = $('live-dot'); if (dot) dot.className = 'dot';
      }
    });
}
setInterval(poll, 1000);
poll();

})();
