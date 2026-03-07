(function() {
'use strict';

const $ = id => document.getElementById(id);
const fmt = (v, d=2) => (v == null || isNaN(v)) ? '--' : Number(v).toFixed(d);
const fmtPx = (v, sym) => {
  if (!v || v <= 0) return '--';
  if (sym === 'btc') return '$' + Number(v).toLocaleString('en-US', {minimumFractionDigits:2, maximumFractionDigits:2});
  if (sym === 'eth') return '$' + Number(v).toLocaleString('en-US', {minimumFractionDigits:2, maximumFractionDigits:2});
  return '$' + Number(v).toFixed(3);
};

// ── AUDIO ──────────────────────────────────────────────────────────────────
let audioCtx = null;
function playWinBell() {
  try {
    if (!audioCtx) audioCtx = new (window.AudioContext || window.webkitAudioContext)();
    const ctx = audioCtx;
    const tone = (freq, t, dur, gain) => {
      const osc = ctx.createOscillator(), g = ctx.createGain();
      osc.connect(g); g.connect(ctx.destination);
      osc.type = 'sine'; osc.frequency.setValueAtTime(freq, t);
      g.gain.setValueAtTime(0, t);
      g.gain.linearRampToValueAtTime(gain, t + 0.01);
      g.gain.exponentialRampToValueAtTime(0.001, t + dur);
      osc.start(t); osc.stop(t + dur);
    };
    const t = ctx.currentTime;
    tone(880, t, 0.6, 0.4); tone(1108, t+0.05, 0.5, 0.25); tone(659, t+0.15, 0.8, 0.2);
  } catch(e) {}
}

// ── WIN FLASH ──────────────────────────────────────────────────────────────
function flashWin(sym, pnl) {
  const el = $('win-flash');
  if (!el) return;
  el.textContent = `✓ WIN  ${sym.toUpperCase().replace('USDT','')}  +${Number(pnl).toFixed(2)}bp`;
  el.classList.add('show');
  setTimeout(() => el.classList.remove('show'), 2500);
}

// ── UPTIME ─────────────────────────────────────────────────────────────────
const startTime = Date.now();
setInterval(() => {
  const s = Math.floor((Date.now() - startTime) / 1000);
  const h = String(Math.floor(s/3600)).padStart(2,'0');
  const m = String(Math.floor((s%3600)/60)).padStart(2,'0');
  const ss = String(s%60).padStart(2,'0');
  const str = `${h}:${m}:${ss}`;
  const el1 = $('tb-uptime'); if (el1) el1.textContent = str;
  const el2 = $('st-uptime'); if (el2) el2.textContent = str;
}, 1000);

// ── READINESS BAR ──────────────────────────────────────────────────────────
function setReadiness(sym, engine, pct) {
  const fill = $(`rfill-${sym}-${engine}`);
  const label = $(`rpct-${sym}-${engine}`);
  if (!fill || !label) return;
  const w = Math.min(100, Math.max(0, pct * 100));
  fill.style.width = w + '%';
  label.textContent = Math.round(w) + '%';
  fill.className = 'rbar-fill ' +
    (w >= 90 ? 'r-ready' : w >= 70 ? 'r-high' : w >= 40 ? 'r-mid' : 'r-low');
}

// ── ENGINE BADGE ──────────────────────────────────────────────────────────
function setEngBadge(id, active, armed) {
  const el = $(id);
  if (!el) return;
  if (active)      { el.textContent = 'ACTIVE'; el.className = 'eng-badge active'; }
  else if (armed)  { el.textContent = 'ARMED';  el.className = 'eng-badge armed';  }
  else             { el.textContent = 'OFF';    el.className = 'eng-badge off';    }
  const cell = el.closest('.eng-cell');
  if (cell) cell.classList.toggle('active-pos', !!active);
}

// ── PNL CELL ──────────────────────────────────────────────────────────────
function setPnl(id, val) {
  const el = $(id); if (!el) return;
  const v = Number(val) || 0;
  const sign = v >= 0 ? '+' : '';
  el.textContent = sign + v.toFixed(2) + 'bp';
  el.className = 'est-val ' + (v > 0 ? 'pos' : v < 0 ? 'neg' : 'dim');
}

// ── CONDITION CHECK ────────────────────────────────────────────────────────
function setCond(id, met, near) {
  const el = $(id); if (!el) return;
  el.className = 'cond-check ' + (met ? 'met' : near ? 'near' : 'off');
}

// ── REGIME ────────────────────────────────────────────────────────────────
const regimeClass = {
  'NEUTRAL': 'rs-neutral', 'GRIND': 'rs-neutral',
  'TRENDING': 'rs-trending', 'EXPANSION': 'rs-trending',
  'BURST': 'rs-burst', 'BREAKOUT': 'rs-burst',
  'COMPRESSION': 'rs-compression',
  'DEAD': 'rs-dead'
};

function setRegime(sym, state, mult) {
  const rs = $(`rs-${sym}`);
  const rm = $(`rm-${sym}`);
  if (rs) {
    rs.textContent = state || 'NEUTRAL';
    rs.className = 'regime-state ' + (regimeClass[state] || 'rs-neutral');
  }
  if (rm) {
    const m = Number(mult) || 1.0;
    rm.textContent = '×' + m.toFixed(2);
    rm.className = 'regime-mult ' + (m >= 1.4 ? 'hi' : m <= 0.6 ? 'lo' : '');
  }
}

// ── PRICE ─────────────────────────────────────────────────────────────────
const lastPrices = {};
function setPrice(sym, val) {
  const el = $(`px-${sym}`); if (!el || !val || val <= 0) return;
  const prev = lastPrices[sym] || val;
  el.textContent = fmtPx(val, sym);
  el.className = 'sym-price ' + (val > prev ? 'up' : val < prev ? 'down' : '');
  lastPrices[sym] = val;
}

// ── TRADE LOG ─────────────────────────────────────────────────────────────
const trades = [];
const tradeStats = { wins: 0, losses: 0, totalPnl: 0 };

function addTrade(sym, layer, pnl) {
  const v = Number(pnl) || 0;
  trades.unshift({ time: new Date().toLocaleTimeString('en-US',{hour12:false}), sym, layer, pnl: v });
  if (trades.length > 100) trades.pop();
  if (v > 0) { tradeStats.wins++; playWinBell(); flashWin(sym, v); }
  else if (v < 0) { tradeStats.losses++; }
  tradeStats.totalPnl += v;
  renderTradeLog();
  updateWinRate();
}

function renderTradeLog() {
  const el = $('trade-log'); if (!el) return;
  if (!trades.length) return;
  el.innerHTML = trades.slice(0,40).map(t => {
    const sign = t.pnl >= 0 ? '+' : '';
    return `<div class="trade-entry">
      <span class="te-time">${t.time}</span>
      <span class="te-sym">${t.sym.replace('USDT','').toUpperCase()}</span>
      <span class="te-layer">${t.layer||'?'}</span>
      <span class="te-pnl ${t.pnl>0?'pos':'neg'}">${sign}${t.pnl.toFixed(2)}bp</span>
    </div>`;
  }).join('');
}

function updateWinRate() {
  const total = tradeStats.wins + tradeStats.losses;
  const wr = total > 0 ? (tradeStats.wins/total*100).toFixed(0) + '%' : '--%';
  const el = $('st-wr'); if (el) { el.textContent = wr; el.className = 'sr-val ' + (tradeStats.wins > tradeStats.losses ? 'pos' : 'neg'); }
}

// ── MAIN UPDATE ───────────────────────────────────────────────────────────
function updateAll(data) {
  if (!data) return;

  // Top bar
  const pnl = Number(data.pnl) || 0;
  const equity = 10000 + pnl;
  const el_eq = $('tb-equity'); if (el_eq) el_eq.textContent = '$' + equity.toFixed(2);
  const el_pnl = $('tb-pnl');
  if (el_pnl) {
    const sign = pnl >= 0 ? '+' : '';
    el_pnl.textContent = sign + pnl.toFixed(2) + 'bp';
    el_pnl.className = 'tb-val ' + (pnl > 0 ? 'pos' : pnl < 0 ? 'neg' : '');
  }
  const el_tr = $('tb-trades'); if (el_tr) el_tr.textContent = data.total_trades || 0;
  const el_pos = $('tb-positions'); if (el_pos) el_pos.textContent = data.open_positions || 0;

  // Stats panel
  const el_sp = $('st-pnl'); if (el_sp) { el_sp.textContent = (pnl>=0?'+':'') + pnl.toFixed(2) + 'bp'; el_sp.className = 'sr-val ' + (pnl>0?'pos':pnl<0?'neg':''); }
  const el_st = $('st-trades'); if (el_st) el_st.textContent = data.total_trades || 0;
  const el_spos = $('st-pos'); if (el_spos) el_spos.textContent = data.open_positions || 0;

  // Prices
  if (data.btc_price > 0) setPrice('btc', data.btc_price);
  if (data.eth_price > 0) setPrice('eth', data.eth_price);
  if (data.sol_price > 0) setPrice('sol', data.sol_price);

  // Per-symbol
  ['btcusdt','ethusdt','solusdt'].forEach(sym => {
    const s = sym.substring(0,3); // btc/eth/sol
    const d = data[sym];
    if (!d) return;

    // Vol ratio and displacement in header
    const vr_el = $(`vr-${s}`); if (vr_el) { const vr = Number(d.vol_ratio)||1; vr_el.textContent = vr.toFixed(3); vr_el.style.color = vr > 1.4 ? 'var(--green)' : vr > 1.2 ? 'var(--yellow)' : 'var(--text)'; }
    const dp_el = $(`dp-${s}`); if (dp_el) { const dp = Number(d.displacement_bp)||0; dp_el.textContent = (dp>=0?'+':'')+dp.toFixed(1)+'bp'; dp_el.style.color = Math.abs(dp) > 20 ? 'var(--yellow)' : 'var(--text)'; }
    const cap_el = $(`cap-${s}`); if (cap_el) cap_el.textContent = Number(d.dynamic_cap_R||0).toFixed(2);
    const reg_el = $(`reg-${s}`); if (reg_el) reg_el.textContent = d.regime_state || '--';

    // Regime panel
    setRegime(s, d.regime_state, d.regime_multiplier);

    // MICRO
    setEngBadge(`eb-${s}-micro`, d.micro_active, false);
    setPnl(`pnl-${s}-micro`, d.micro_total_pnl_bp);
    const tr_m = $(`trades-${s}-micro`); if (tr_m) tr_m.textContent = d.micro_total_trades || 0;

    // STRUCTURAL
    setEngBadge(`eb-${s}-structural`, d.structural_active, d.structural_readiness > 0.7);
    setPnl(`pnl-${s}-structural`, d.structural_total_pnl_bp);
    const tr_st = $(`trades-${s}-structural`); if (tr_st) tr_st.textContent = d.structural_total_trades || 0;
    setReadiness(s, 'structural', d.structural_readiness || 0);

    // CONVEX
    setEngBadge(`eb-${s}-convex`, d.convex_active, d.convex_readiness > 0.7);
    setPnl(`pnl-${s}-convex`, d.convex_total_pnl_bp);
    const accel_el = $(`accel-${s}`); if (accel_el) { const a = Number(d.acceleration_bp)||0; accel_el.textContent = (a>=0?'+':'')+a.toFixed(1)+'bp'; accel_el.className = 'est-val ' + (Math.abs(a)>=15?'pos':'dim'); }
    setReadiness(s, 'convex', d.convex_readiness || 0);

    // COMPRESSION
    const cticks = Number(d.compression_ticks) || 0;
    setEngBadge(`eb-${s}-compression`, d.compression_active, cticks >= 80);
    setPnl(`pnl-${s}-compression`, d.compression_total_pnl_bp);
    const ct_el = $(`cticks-${s}`); if (ct_el) ct_el.textContent = cticks + '/100';
    setReadiness(s, 'compression', d.compression_readiness || 0);
  });

  // BTC signal conditions panel (right panel)
  const bd = data.btcusdt;
  if (bd) {
    const vol = Number(bd.vol_ratio)||1;
    const disp = Math.abs(Number(bd.displacement_bp)||0);
    const build = Number(bd.buildup_ticks)||0;
    const accel = Math.abs(Number(bd.acceleration_bp)||0);
    const comp = Number(bd.compression_ticks)||0;

    const cv_vol = $('cv-btc-vol'); if (cv_vol) cv_vol.textContent = vol.toFixed(3) + ' / 1.4';
    setCond('cc-btc-vol', vol >= 1.4, vol >= 1.2);

    const cv_disp = $('cv-btc-disp'); if (cv_disp) cv_disp.textContent = disp.toFixed(1) + ' / 20bp';
    setCond('cc-btc-disp', disp >= 20, disp >= 12);

    const cv_build = $('cv-btc-build'); if (cv_build) cv_build.textContent = build + ' / 40';
    setCond('cc-btc-build', build >= 40, build >= 25);

    const cv_accel = $('cv-btc-accel'); if (cv_accel) cv_accel.textContent = accel.toFixed(1) + ' / 15bp';
    setCond('cc-btc-accel', accel >= 15, accel >= 8);

    const cv_comp = $('cv-btc-comp'); if (cv_comp) cv_comp.textContent = comp + ' / 100';
    setCond('cc-btc-comp', comp >= 100, comp >= 70);
  }
}

// ── HTTP POLL ─────────────────────────────────────────────────────────────
let pollFails = 0;
function poll() {
  fetch('/api/state')
    .then(r => r.json())
    .then(data => {
      pollFails = 0;
      updateAll(data);
      const cb = $('conn-badge'); if (cb) { cb.textContent = 'LIVE'; cb.className = 'badge badge-conn'; }
      const dot = $('live-dot'); if (dot) dot.className = 'dot live';
    })
    .catch(() => {
      pollFails++;
      if (pollFails > 3) {
        const cb = $('conn-badge'); if (cb) { cb.textContent = 'OFFLINE'; cb.className = 'badge badge-disc'; }
        const dot = $('live-dot'); if (dot) dot.className = 'dot';
      }
    });
}

setInterval(poll, 1000);
poll();

// ── HANDLE POSITION_EXIT events from log if WS available ──────────────────
// The HTTP poll covers all state. But if future WS is added, hook here.
window._chimera_trade = (sym, layer, pnl) => addTrade(sym, layer, pnl);

})();
