// CHIMERA app.js — Omega-style rebuild
const STORAGE_KEY = 'chimera_trades_v3';
const BOOT_TS = Date.now();

const SYMBOLS = [
  { short: 'BTC',  full: 'btcusdt'  },
  { short: 'ETH',  full: 'ethusdt'  },
  { short: 'SOL',  full: 'solusdt'  },
  { short: 'BNB',  full: 'bnbusdt'  },
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

// AUDIO
function unlockAudio() {
  if (audioUnlocked) return;
  try {
    audioCtx = new (window.AudioContext || window.webkitAudioContext)();
    const buf = audioCtx.createBuffer(1, 1, 22050);
    const src = audioCtx.createBufferSource();
    src.buffer = buf; src.connect(audioCtx.destination); src.start(0);
    audioUnlocked = true;
    const btn = document.getElementById('audio-unlock');
    if (btn) { btn.textContent = 'BELL ON'; btn.style.color = 'var(--green)'; btn.style.borderColor = 'var(--green)'; btn.style.background = 'rgba(0,230,118,.1)'; }
  } catch(e) {}
}

function playWin() {
  if (!audioUnlocked || !audioCtx) return;
  try {
    const now = audioCtx.currentTime;
    const comp = audioCtx.createDynamicsCompressor();
    comp.threshold.value = -3; comp.ratio.value = 4;
    comp.attack.value = 0.003; comp.release.value = 0.1;
    comp.connect(audioCtx.destination);
    [[0, 880, 1040], [0.22, 1100, 1320]].forEach(([t, f1, f2]) => {
      [f1, f2].forEach((freq, i) => {
        const osc = audioCtx.createOscillator(), gain = audioCtx.createGain();
        osc.connect(gain); gain.connect(comp);
        osc.type = 'sine'; osc.frequency.setValueAtTime(freq, now + t);
        gain.gain.setValueAtTime(0, now + t);
        gain.gain.linearRampToValueAtTime(i === 0 ? 1.8 : 0.9, now + t + 0.008);
        gain.gain.exponentialRampToValueAtTime(0.001, now + t + 1.4);
        osc.start(now + t); osc.stop(now + t + 1.5);
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
  el.textContent = sym + '  +' + (+pnl).toFixed(2) + 'bp';
  el.classList.add('show');
  setTimeout(() => el.classList.remove('show'), 2200);
}

// STORAGE
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

function currentSessionTrades() {
  const all = [];
  for (const t of localTrades) {
    if (t.s === 'SESSION' || t.e === 'START') break;
    all.push(t);
  }
  return all;
}

function mergeTrades(serverLog, isBootLoad) {
  if (!serverLog || !serverLog.length) return;
  const before = localTrades.length;
  const existing = new Set(localTrades.map(t => t.t+'|'+t.s+'|'+t.e+'|'+t.p));
  let newCount = 0;
  serverLog.forEach(tr => {
    const key = tr.t+'|'+tr.s+'|'+tr.e+'|'+tr.p;
    if (!existing.has(key)) {
      localTrades.unshift(tr); existing.add(key); newCount++;
      if (!isBootLoad) {
        const tsStr = tr.t
          ? (tr.t.length < 12
              ? new Date().toISOString().slice(0,10) + 'T' + tr.t + 'Z'
              : (tr.t.endsWith('Z') || tr.t.includes('+') ? tr.t : tr.t + 'Z'))
          : null;
        const tradeAge = tsStr ? (Date.now() - new Date(tsStr).getTime()) : 99999;
        const isFresh = tradeAge < 60000;
        if (+tr.p > 0) { wins++; if (isFresh) { playWin(); flashWin(tr.s, tr.p); } }
        else if (+tr.p < 0) { losses++; if (isFresh) playLoss(); }
        else wins++;
      } else {
        if (+tr.p > 0) wins++; else if (+tr.p < 0) losses++;
      }
    }
  });
  if (newCount > 0 || before === 0) {
    localTrades = localTrades.slice(0, 200);
    saveTrades(); renderTradeLog(); updateWinRate();
  }
}

// HELPERS
const $ = id => document.getElementById(id);
const set = (id, val) => { const el = $(id); if (el) el.textContent = val; };
const fmtPnl = v => (v >= 0 ? '+' : '') + (+v).toFixed(2) + 'bp';
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
  if (s === 'SOL' || s === 'LINK' || s === 'POL') return '$' + (+p).toFixed(3);
  if (s === 'AVAX') return '$' + (+p).toFixed(2);
  return '$' + (+p).toLocaleString('en-US', {minimumFractionDigits:2,maximumFractionDigits:2});
}

function updateWinRate() {
  const t = wins + losses;
  const wr = t > 0 ? (wins / t * 100).toFixed(0) + '%' : '--%';
  const wrEl = $('ts-wr');
  if (wrEl) { wrEl.textContent = wr; wrEl.className = 'ss-val ' + (t > 0 ? (wins >= losses ? 'pos' : 'neg') : ''); }
}

function updateUptime() {
  if (!uptimeStart) return;
  const s = Math.floor((Date.now() - uptimeStart) / 1000);
  const h = Math.floor(s / 3600), m = Math.floor((s % 3600) / 60), sec = s % 60;
  set('tb-uptime', String(h).padStart(2,'0')+':'+String(m).padStart(2,'0')+':'+String(sec).padStart(2,'0'));
}

function reasonClass(r) {
  if (!r) return 'timeout';
  const rl = r.toLowerCase();
  if (rl === 'tp') return 'tp';
  if (rl === 'sl' || rl === 'sc') return 'sl';
  if (rl.startsWith('trail')) return 'trail';
  return 'timeout';
}

function normalizeReason(r) {
  if (!r) return 'TO';
  const ru = r.toUpperCase();
  if (ru === 'SC') return 'SL';
  if (ru.startsWith('TRAIL')) return 'TRAIL';
  if (ru === 'TIMEOUT') return 'TO';
  return ru;
}

function renderTradeLog() {
  let totalPnl = 0, winPnl = 0, lossPnl = 0, winCount = 0, lossCount = 0;
  let tp = 0, sl = 0, trail = 0, timeout = 0;
  currentSessionTrades().forEach(t => {
    const p = +t.p || 0; totalPnl += p;
    const why = normalizeReason(t.why || t.reason || '');
    if (why === 'TP') tp++; else if (why === 'SL') sl++; else if (why === 'TRAIL') trail++; else timeout++;
    if (p >= 0) { winPnl += p; winCount++; } else { lossPnl += p; lossCount++; }
  });
  const avgWin  = winCount  > 0 ? winPnl  / winCount  : null;
  const avgLoss = lossCount > 0 ? lossPnl / lossCount : null;
  const total   = winCount + lossCount;
  const wr      = total > 0 ? winCount / total : null;
  const exp     = (wr !== null && avgWin !== null && avgLoss !== null) ? (wr * avgWin + (1-wr) * avgLoss) : null;

  // Session stats
  set('ts-count', total);
  set('ts-wins', winCount); set('ts-losses', lossCount);
  const pnlEl = $('ts-pnl');
  if (pnlEl) { pnlEl.textContent = fmtPnl(totalPnl); pnlEl.className = 'ss-val ' + (totalPnl >= 0 ? 'pos' : 'neg'); }
  const wrEl2 = $('ts-wr');
  if (wrEl2) { wrEl2.textContent = wr !== null ? (wr*100).toFixed(0)+'%' : '--%'; wrEl2.className = 'ss-val ' + (wr !== null ? (wr >= 0.5 ? 'pos' : 'neg') : ''); }
  const awEl = $('ts-avgwin'); if (awEl) awEl.textContent = avgWin !== null ? '+'+avgWin.toFixed(2)+'bp' : '--';
  const alEl = $('ts-avgloss'); if (alEl) alEl.textContent = avgLoss !== null ? avgLoss.toFixed(2)+'bp' : '--';
  const expEl = $('ts-exp');
  if (expEl) { expEl.textContent = exp !== null ? (exp>=0?'+':'')+exp.toFixed(2)+'bp' : '--'; expEl.className = 'ss-val '+(exp!==null?(exp>=0?'pos':'neg'):''); }
  set('st-tp', tp); set('st-sl', sl); set('st-trail', trail); set('st-timeout', timeout);

  // Trade log header
  set('tp-wins', winCount); set('tp-losses', lossCount);
  const tpWr = $('tp-wr');
  if (tpWr) { tpWr.textContent = wr !== null ? (wr*100).toFixed(0)+'%' : '--%'; tpWr.className = wr !== null ? (wr >= 0.5 ? 'pos' : 'neg') : ''; }
  const tpPnl = $('tp-pnl');
  if (tpPnl) { tpPnl.textContent = fmtPnl(totalPnl); tpPnl.className = totalPnl >= 0 ? 'pos' : 'neg'; }

  // Trade rows
  const body = $('btm-trade-rows');
  if (!body) return;
  const trades = currentSessionTrades();
  if (!trades.length) {
    body.innerHTML = '<div class="tl-empty">Waiting for first trade...</div>';
    return;
  }
  body.innerHTML = trades.slice(0, 50).map(tr => {
    const p = +tr.p || 0, isWin = p >= 0;
    const sym = (tr.s||'').replace('USDT','').replace('/','');
    const eng = (tr.e||'?').toUpperCase();
    const why = normalizeReason(tr.why || tr.reason || '?');
    const whyCls = why==='TP'?'why-tp':why==='SL'?'why-sl':why==='TRAIL'?'why-trail':'why-to';
    const time = tr.t ? (tr.t.length > 10 ? tr.t.substring(11,16) : tr.t) : '--';
    const mfe = tr.mfe != null ? '+' + (+tr.mfe).toFixed(1)+'bp' : '--';
    const mae = tr.mae != null ? (+tr.mae).toFixed(1)+'bp' : '--';
    const hold = tr.hold != null ? fmtHold(tr.hold) : '--';
    return '<div class="tl-row '+(isWin?'tl-win':'tl-loss')+'">'
      +'<span class="tl-time">'+time+'</span>'
      +'<span class="tl-sym">'+sym+'</span>'
      +'<span class="tl-eng">'+eng+'</span>'
      +'<span class="tl-why '+whyCls+'">'+why+'</span>'
      +'<span class="tl-pnl '+(isWin?'pos':'neg')+'">'+(isWin?'+':'')+p.toFixed(2)+'bp</span>'
      +'<span class="tl-mfe">'+mfe+'</span>'
      +'<span class="tl-mae">'+mae+'</span>'
      +'<span class="tl-hold">'+hold+'</span>'
      +'</div>';
  }).join('');
}

// CARD READINESS GRADIENT
// Score 0-1 based on vol_ratio + displacement + compression_ticks
// Maps to colour: grey(0) -> yellow(0.4) -> cyan(0.7) -> green(1.0)
function readinessColor(d) {
  if (!d) return 'var(--dim)';
  const vol    = Math.min(1, Math.max(0, ((d.vol_ratio||1) - 1.0) / 0.8));
  const disp   = Math.min(1, Math.abs(d.displacement_bp||0) / 30);
  const comp   = Math.min(1, (d.compression_ticks||0) / 100);
  const score  = vol * 0.5 + disp * 0.3 + comp * 0.2;
  if (score < 0.2) return 'var(--dim)';
  if (score < 0.45) return 'rgba(255,214,0,.6)';
  if (score < 0.7)  return 'rgba(0,212,255,.7)';
  return 'rgba(0,230,118,.9)';
}

function regimePillClass(state) {
  if (!state) return 'rp-neutral';
  const s = state.toUpperCase();
  if (s.includes('BREAKOUT') || s.includes('BURST')) return 'rp-breakout';
  if (s.includes('BUILDUP') || s.includes('TREND')) return 'rp-buildup';
  if (s.includes('COMPRESSION')) return 'rp-compression';
  if (s.includes('DEAD')) return 'rp-dead';
  return 'rp-neutral';
}

// MAIN UPDATE
let firstPoll = true;
let lastKnownUptimeHours = null;

function updateAll(data) {
  if (!data) return;
  if (!uptimeStart) uptimeStart = Date.now();

  // Restart detection
  const serverUptime = data.uptime_hours || 0;
  if (lastKnownUptimeHours !== null && serverUptime < lastKnownUptimeHours - 0.01) {
    localTrades = [];
    try { localStorage.removeItem(STORAGE_KEY); } catch(e) {}
    uptimeStart = Date.now();
    firstPoll = true;
  }
  lastKnownUptimeHours = serverUptime;

  // Topbar
  const pnl = data.pnl || 0;
  const pnlEl = $('tb-pnl');
  if (pnlEl) { pnlEl.textContent = fmtPnl(pnl); pnlEl.className = 'tb-val '+(pnl>0?'pos':pnl<0?'neg':''); }
  const equity = $('tb-equity');
  if (equity) equity.textContent = '$'+(10000+pnl).toLocaleString('en-US',{minimumFractionDigits:2,maximumFractionDigits:2});
  set('tb-trades', data.total_trades || 0);
  set('tb-positions', data.open_positions || 0);
  if (data.build_ver) { const bv = $('build-ver'); if (bv) bv.textContent = 'v' + data.build_ver; }
  const lat = data.latency_p95 || 0;
  const latEl = $('tb-latency');
  if (latEl) {
    latEl.textContent = lat > 0 ? lat.toFixed(1)+'ms' : '--ms';
    latEl.className = 'tb-val accent ' + (lat <= 0 ? '' : lat < 25 ? 'pos' : lat < 50 ? '' : 'neg');
  }

  // Per-symbol cards
  SYMBOLS.forEach(({ short, full }) => {
    const sym = short.toLowerCase();
    const d = data[full];
    const px = data[full+'_price'] || 0;

    // Price
    const pxEl = $('px-'+sym);
    if (pxEl) {
      pxEl.textContent = fmtPrice(px, short);
      pxEl.className = 'sym-price' + (px > lastPrices[sym] ? ' up' : px < lastPrices[sym] ? ' down' : '');
    }
    lastPrices[sym] = px;

    if (!d) return;

    // Readiness bar
    const bar = $('bar-'+sym);
    if (bar) bar.style.background = readinessColor(d);

    // Regime pill
    const state = d.regime_state || 'NEUTRAL';
    const reg = $('reg-'+sym);
    if (reg) { reg.textContent = state; reg.className = 'sym-regime-pill ' + regimePillClass(state); }

    // Metrics
    set('vr-'+sym,  d.vol_ratio != null ? d.vol_ratio.toFixed(2) : '--');
    set('dp-'+sym,  d.displacement_bp != null ? d.displacement_bp.toFixed(1)+'bp' : '--');
    set('cap-'+sym, d.dynamic_cap_R != null ? d.dynamic_cap_R.toFixed(1)+'R' : '--');

    // Card active state
    const card = $('sb-'+sym);
    if (card) card.className = 'sym-card' + (d.micro_active ? ' active' : '');

    // Engine dots
    const engs = [
      ['micro',       d.micro_active],
      ['structural',  d.structural_active],
      ['convex',      d.convex_active],
      ['compression', d.compression_active],
      ['obi',         d.obi_active],
      ['afe',         d.afe_active],
      ['pce',         d.pce_active],
    ];
    let anyActive = false;
    engs.forEach(([eng, active]) => {
      const dot = $('ed-'+sym+'-'+eng);
      if (dot) dot.className = 'eng-dot' + (active ? ' active' : '');
      if (active) anyActive = true;
    });
    const lbl = $('edl-'+sym);
    if (lbl) lbl.textContent = anyActive ? 'ACTIVE' : 'FLAT';

    // Mini P&L + trades
    const symTrades = currentSessionTrades().filter(t => (t.s||'').replace('USDT','').toUpperCase() === short);
    const symPnl = symTrades.reduce((acc, t) => acc + (+t.p||0), 0);
    const miniPnl = $('mini-pnl-'+sym);
    if (miniPnl) { miniPnl.textContent = (symPnl>=0?'+':'')+symPnl.toFixed(2)+'bp'; miniPnl.className = 'sym-pnl '+(symPnl>0?'pos':symPnl<0?'neg':'zero'); }
    set('mini-t-'+sym, symTrades.length || 0);
  });

  // Session by-layer
  if (data.session && data.session.by_layer) {
    const bl = $('st-by-layer');
    const s = data.session;
    if (bl && s.by_layer && s.by_layer.length) {
      bl.innerHTML = s.by_layer.map(l =>
        '<div class="by-eng-row">'
        +'<span class="be-name">'+l.name+'</span>'
        +'<div class="be-stats">'
        +'<span>'+l.trades+'T</span>'
        +'<span class="be-wr '+(l.wr>=50?'good':'bad')+'">'+Math.round(l.wr)+'%</span>'
        +'<span class="be-pnl '+(l.pnl>=0?'pos':'neg')+'">'+(l.pnl>=0?'+':'')+Number(l.pnl).toFixed(2)+'bp</span>'
        +'</div></div>'
      ).join('');
    }
    // Exit counts
    set('st-tp', s.tp_exits||0); set('st-sl', s.sl_exits||0);
    set('st-trail', s.trail_exits||0); set('st-timeout', s.timeout_exits||0);
  }

  if (data.trade_log) mergeTrades(data.trade_log, firstPoll);
  firstPoll = false;
}

// POLL
let connected = false;
let pollErrors = 0;
let lastPollOk = null;

function fmtAgo(ts) {
  if (!ts) return 'never';
  const s = Math.floor((Date.now()-ts)/1000);
  if (s < 60) return s+'s ago';
  return Math.floor(s/60)+'m'+(s%60)+'s ago';
}

function setPollOk() {
  pollErrors = 0; lastPollOk = Date.now(); connected = true;
  const cb = $('conn-badge');
  if (cb) { cb.textContent = 'LIVE'; cb.className = 'badge badge-conn'; }
  const dot = $('live-dot'); if (dot) dot.className = 'dot live';
  const banner = $('poll-error'); if (banner) banner.classList.remove('show');
}

function setPollError(reason) {
  pollErrors++; connected = false;
  const cb = $('conn-badge'); if (cb) { cb.textContent = 'OFFLINE'; cb.className = 'badge badge-disc'; }
  const dot = $('live-dot'); if (dot) dot.className = 'dot';
  const banner = $('poll-error'); if (banner) banner.classList.add('show');
  const msg = $('poll-error-msg'); if (msg) msg.textContent = reason;
  const cnt = $('poll-error-count'); if (cnt) cnt.textContent = 'ERR'+pollErrors;
  const et = $('poll-error-time'); if (et) et.textContent = 'last ok: '+fmtAgo(lastPollOk);
}

async function poll() {
  let res;
  try {
    res = await fetch('/api/state', { cache:'no-store', signal:AbortSignal.timeout(4000) });
  } catch(e) {
    setPollError(e.name === 'TimeoutError' ? 'Fetch timeout' : 'Network error — backend down?');
    return;
  }
  if (!res.ok) { setPollError('HTTP '+res.status); return; }
  let data;
  try { data = await res.json(); }
  catch(e) { setPollError('JSON parse error'); return; }
  setPollOk();
  updateAll(data);
}

setInterval(() => {
  if (pollErrors > 0) {
    const et = $('poll-error-time'); if (et) et.textContent = 'last ok: '+fmtAgo(lastPollOk);
  }
}, 1000);

// INIT
loadTrades(); renderTradeLog(); updateWinRate();
wins = 0; losses = 0;
currentSessionTrades().forEach(t => { if (+t.p > 0) wins++; else if (+t.p < 0) losses++; });
updateWinRate();
poll();
setInterval(poll, 1000);
setInterval(updateUptime, 1000);
