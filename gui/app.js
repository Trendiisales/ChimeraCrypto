// CHIMERA app.js — Omega-style rebuild
const STORAGE_KEY = 'chimera_trades_v3';
const CLEAR_KEY   = 'chimera_cleared_at';
const BOOT_TS = Date.now();
let sessionClearedAt = 0;  // epoch ms — trades with server-side index <= this are suppressed

const SYMBOLS = [
  { short: 'BTC',  full: 'btcusdt'  },
  { short: 'ETH',  full: 'ethusdt'  },
  { short: 'SOL',  full: 'solusdt'  },
  { short: 'BNB',  full: 'bnbusdt'  },
  { short: 'AVAX', full: 'avaxusdt' },
  { short: 'LINK', full: 'linkusdt' },
  { short: 'XRP',  full: 'xrpusdt'  },
  { short: 'DOGE', full: 'dogeusdt' },
];

let localTrades = [];
let audioCtx = null;
let audioUnlocked = false;
let lastPrices  = {};
let sessionHi   = {};
let sessionLo   = {};
SYMBOLS.forEach(s => {
  const k = s.short.toLowerCase();
  lastPrices[k] = 0;
  sessionHi[k]  = 0;
  sessionLo[k]  = Infinity;
});
let wins = 0, losses = 0;
let uptimeStart = null;

// AUDIO
function unlockAudio() {
  try {
    if (!audioCtx) {
      audioCtx = new (window.AudioContext || window.webkitAudioContext)();
    }
    if (audioCtx.state === 'suspended') {
      audioCtx.resume();
    }
    // Play a silent buffer to fully unlock
    const buf = audioCtx.createBuffer(1, 1, 22050);
    const src = audioCtx.createBufferSource();
    src.buffer = buf; src.connect(audioCtx.destination); src.start(0);
    audioUnlocked = true;
    const btn = document.getElementById('audio-unlock');
    if (btn) {
      btn.textContent = 'BELL ON';
      btn.style.color = 'var(--green)';
      btn.style.borderColor = 'var(--green)';
      btn.style.background = 'rgba(0,230,118,.1)';
    }
  } catch(e) { console.warn('Audio unlock failed:', e); }
}

function playWin() {
  if (!audioCtx) return;
  if (audioCtx.state === 'suspended') { audioCtx.resume(); }
  if (!audioUnlocked) return;
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
  if (!audioCtx) return;
  if (audioCtx.state === 'suspended') { audioCtx.resume(); }
  if (!audioUnlocked) return;
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
  try { const c = localStorage.getItem(CLEAR_KEY); if (c) sessionClearedAt = parseInt(c, 10); } catch(e) {}
}
function saveTrades() {
  try { localStorage.setItem(STORAGE_KEY, JSON.stringify(localTrades.slice(0, 200))); } catch(e) {}
}
window.clearTrades = function() {
  // Suppress all trades currently known to the server so they don't re-appear after clear
  const last = window._lastApiData;
  if (last && last.trade_log) {
    last.trade_log.forEach(tr => {
      _suppressedKeys.add(tr.t+'|'+tr.s+'|'+tr.e+'|'+tr.p);
    });
    _saveSuppressed();
  }
  localTrades = []; wins = 0; losses = 0;
  sessionClearedAt = Date.now();
  try { localStorage.setItem(CLEAR_KEY, String(sessionClearedAt)); } catch(e) {}
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

// Keys of server trades that existed when user last hit CLEAR — always skip these
const _suppressedKeys = new Set();
let   _suppressLoaded = false;
const SUPPRESS_KEY = 'chimera_suppressed_v1';

function _loadSuppressed() {
  if (_suppressLoaded) return;
  _suppressLoaded = true;
  try { const r = localStorage.getItem(SUPPRESS_KEY); if (r) JSON.parse(r).forEach(k => _suppressedKeys.add(k)); } catch(e) {}
}
function _saveSuppressed() {
  try { localStorage.setItem(SUPPRESS_KEY, JSON.stringify([..._suppressedKeys].slice(0,500))); } catch(e) {}
}

function mergeTrades(serverLog, isBootLoad) {
  _loadSuppressed();
  if (!serverLog || !serverLog.length) return;
  const before = localTrades.length;
  const existing = new Set(localTrades.map(t => t.t+'|'+t.s+'|'+t.e+'|'+t.p));
  let newCount = 0;
  serverLog.forEach(tr => {
    const key = tr.t+'|'+tr.s+'|'+tr.e+'|'+tr.p;
    if (_suppressedKeys.has(key)) return;  // user cleared these
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
  if (s === 'SOL' || s === 'LINK' || s === 'XRP' || s === 'DOGE') return '$' + (+p).toFixed(4);
  if (s === 'AVAX') return '$' + (+p).toFixed(2);
  return '$' + (+p).toLocaleString('en-US', {minimumFractionDigits:2,maximumFractionDigits:2});
}

function updateWinRate() {
  // Recalculate from current session only — ignore historical sessions
  const sess = currentSessionTrades().filter(t => t.s !== 'SESSION');
  wins = 0; losses = 0;
  sess.forEach(t => { if (+t.p > 0) wins++; else if (+t.p < 0) losses++; else wins++; });
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

// CARD READINESS — uses engine-specific readiness scores from backend
// structural_readiness, convex_readiness, compression_readiness (0.0-1.0)
// Best score of any active engine = overall card readiness

function bestReadiness(d) {
  if (!d) return 0;
  // Score ACTIVE engines only (structural/convex/compression are disabled)
  // LEADLAG: BTC 180ms move vs 3.5bp threshold
  const btcMove = Math.abs(d.btc_move_bp || 0);
  const leadlag_pct = Math.min(1.0, btcMove / 3.5);
  // VWAP: price below session VWAP vs 25bp entry threshold
  const vwap_dev = d.vwap_deviation_bp || 0;
  const vwap_rdy = (d.vwap_ready === true || d.vwap_ready === 'true');
  const vwap_pct = vwap_rdy && vwap_dev > 0 ? Math.min(1.0, vwap_dev / 25.0) : 0;
  // LIQ: notional building toward $1M threshold
  const liq_pct = Math.min(1.0, (d.liq_notional || 0) / 1000000) * 0.7;
  // MM: slow book imbalance EMA toward 0.25
  const mm_pct = Math.min(1.0, (d.mm_imbal_ema || 0) / 0.25) * 0.5;
  // Bracket range
  const bracket_pct = d.bracket_range_pct || 0;
  return Math.max(leadlag_pct, vwap_pct, liq_pct, mm_pct, bracket_pct);
}

function readinessColor(score) {
  if (score < 0.15) return { bg: 'var(--dim)',              fill: 'var(--dim)' };
  if (score < 0.40) return { bg: 'rgba(255,214,0,.15)',     fill: 'rgba(255,214,0,.8)' };
  if (score < 0.65) return { bg: 'rgba(0,212,255,.12)',     fill: 'rgba(0,212,255,.9)' };
  if (score < 0.85) return { bg: 'rgba(0,230,118,.15)',     fill: 'rgba(0,230,118,.9)' };
  return             { bg: 'rgba(0,230,118,.25)',           fill: '#00e676' };
}

function updateCardReadiness(sym, d) {
  const score  = bestReadiness(d);
  const pct    = Math.round(score * 100);
  const colors = readinessColor(score);

  // Bar background tint
  const bar = $('bar-'+sym);
  if (bar) bar.style.background = colors.bg;

  // Fill width
  const fill = $('bar-fill-'+sym);
  if (fill) {
    fill.style.width      = pct + '%';
    fill.style.background = colors.fill;
    // Pulse animation when >= 80%
    fill.style.boxShadow  = pct >= 80 ? '0 0 8px ' + colors.fill : 'none';
  }

  // Percentage label — hide at 0, colour-code as it rises
  const lbl = $('rdy-'+sym);
  if (lbl) {
    if (pct <= 5) {
      lbl.textContent = '';
    } else {
      lbl.textContent = pct + '%';
      lbl.style.color = pct >= 80 ? 'var(--green)'
                      : pct >= 50 ? 'var(--accent)'
                      : pct >= 25 ? 'var(--yellow)'
                      : 'var(--muted)';
    }
  }
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

// LIVE POSITIONS PANEL
function updateLivePositions(data) {
  const list = document.getElementById("live-positions-list");
  if (!list) return;
  const positions = [];
  const syms = ["btcusdt","ethusdt","solusdt","bnbusdt","avaxusdt","linkusdt","xrpusdt","dogeusdt"];
  syms.forEach(sym => {
    const d = data[sym]; if (!d) return;
    const short = sym.replace("usdt","").toUpperCase();
    if (d.bracket_active) positions.push({sym:short,eng:"BRACKET",move:d.bracket_move_bp||0,mfe:d.bracket_mfe_bp||0,trail_floor:d.bracket_trail_floor||-9999,trail_armed:d.bracket_trail_armed||false,sl_bp:28,trail_arm_bp:40});
    if (d.basis_active)   positions.push({sym:short,eng:"BASIS",  move:d.basis_move_bp||0,  mfe:d.basis_mfe_bp||0,  trail_floor:d.basis_trail_floor||-9999,  trail_armed:d.basis_trail_armed||false,  sl_bp:12,trail_arm_bp:20});
    if (d.fundwin_active) positions.push({sym:short,eng:"FUND-WIN",move:d.fundwin_move_bp||0,mfe:d.fundwin_mfe_bp||0,trail_floor:-9999,trail_armed:(d.fundwin_mfe_bp||0)>=30,sl_bp:20,trail_arm_bp:30});
    if (d.liq_active)     positions.push({sym:short,eng:"LIQ",    move:d.liq_move_bp||0,    mfe:d.liq_mfe_bp||0,    trail_floor:-9999,                        trail_armed:(d.liq_mfe_bp||0)>=30,      sl_bp:20,trail_arm_bp:30});
  });
  if (!positions.length) {
    list.innerHTML='<div class="live-pos-empty">No open positions — waiting for LIQ / BRACKET / BASIS setup</div>';
    return;
  }
  const cost=15;
  list.innerHTML=positions.map(p=>{
    const mv=+p.move,mfe=+p.mfe,net=mv-cost;
    const maxBp=Math.max(200,mfe+50);
    const movePct=Math.max(0,Math.min(100,mv/maxBp*100));
    const trailPct=p.trail_armed&&p.trail_floor>-9999?Math.max(0,Math.min(100,p.trail_floor/maxBp*100)):0;
    const barColor=p.trail_armed?(mv>p.trail_floor+5?"var(--green)":"var(--yellow)"):(mv>0?"rgba(0,212,255,.7)":"var(--red)");
    let trailTxt="",trailClass="";
    if(p.trail_armed&&p.trail_floor>-9999){trailTxt="TRAIL FLOOR: +"+(p.trail_floor).toFixed(0)+"bp (net +"+(Math.max(0,p.trail_floor-cost)).toFixed(0)+"bp)";trailClass=mv>p.trail_floor+15?"locked":"armed";}
    else if(mfe>=p.trail_arm_bp*0.6){trailTxt="arming at +"+p.trail_arm_bp+"bp...";trailClass="armed";}
    else{trailTxt="SL -"+p.sl_bp+"bp  |  trail arms at +"+p.trail_arm_bp+"bp";}
    return `<div class="live-pos">
      <span class="lp-sym">${p.sym}</span>
      <span class="lp-eng">${p.eng}</span>
      <span class="lp-move ${mv>=0?"pos":"neg"}">${mv>=0?"+":""}${mv.toFixed(1)}bp</span>
      <span class="lp-mfe">peak +${mfe.toFixed(1)}bp</span>
      <span class="lp-move ${net>=0?"pos":"neg"}" style="font-size:11px">${net>=0?"+":""}${net.toFixed(1)}bp net</span>
      <span class="lp-trail ${trailClass}">${trailTxt}</span>
      <div style="flex:1"><div class="lp-bar-wrap">
        <div class="lp-bar-fill" style="width:${movePct}%;background:${barColor}"></div>
        ${trailPct>0?`<div class="lp-bar-trail" style="left:${trailPct}%"></div>`:""}
      </div></div></div>`;
  }).join("");
}

function updateAll(data) {
  if (!data) return;
  window._lastApiData = data;
  if (!uptimeStart) uptimeStart = Date.now();

  // Restart detection
  const serverUptime = data.uptime_hours || 0;
  if (lastKnownUptimeHours !== null && serverUptime < lastKnownUptimeHours - 0.01) {
    localTrades = [];
    _suppressedKeys.clear();
    SYMBOLS.forEach(s => { const k = s.short.toLowerCase(); sessionHi[k] = 0; sessionLo[k] = Infinity; });
    try { localStorage.removeItem(STORAGE_KEY); localStorage.removeItem(SUPPRESS_KEY); localStorage.removeItem(CLEAR_KEY); } catch(e) {}
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
    if (px > 0) {
      if (px > sessionHi[sym]) sessionHi[sym] = px;
      if (px < sessionLo[sym]) sessionLo[sym] = px;
    }

    if (!d) return;

    // Readiness bar
    updateCardReadiness(sym, d);

    // Regime pill
    const state = d.regime_state || 'NEUTRAL';
    const reg = $('reg-'+sym);
    if (reg) { reg.textContent = state; reg.className = 'sym-regime-pill ' + regimePillClass(state); }

    // Metrics
    set('vr-'+sym,  d.vol_ratio != null ? d.vol_ratio.toFixed(2) : '--');
    set('dp-'+sym,  d.displacement_bp != null ? d.displacement_bp.toFixed(1)+'bp' : '--');
    set('cap-'+sym, d.dynamic_cap_R != null ? d.dynamic_cap_R.toFixed(1)+'R' : '--');

    // Row 2: funding, basis, liquidation notional, session PnL
    const fnEl = $('fn-'+sym);
    if (fnEl) {
      const fr = d.perp_funding_rate;
      if (fr != null && fr !== 0) {
        const frBp = (fr * 10000).toFixed(2);
        fnEl.textContent = (fr>=0?'+':'')+frBp+'bp';
        fnEl.className = 'sm2-val '+(fr>0.0003?'neg':fr<-0.0002?'pos':'zero');
      } else { fnEl.textContent='--'; fnEl.className='sm2-val zero'; }
    }
    const bsEl = $('bs-'+sym);
    if (bsEl) {
      const basis = d.perp_basis_bp;
      if (basis != null && Math.abs(basis) > 0.05) {
        bsEl.textContent = (basis>=0?'+':'')+basis.toFixed(1)+'bp';
        bsEl.className = 'sm2-val '+(basis>5?'neg':basis<-5?'pos':'zero');
      } else { bsEl.textContent='--'; bsEl.className='sm2-val zero'; }
    }
    const lqEl = $('lq-'+sym);
    if (lqEl) {
      const liq = d.liq_notional;
      if (liq != null && liq >= 500) {
        lqEl.textContent = liq>=1000000?'$'+(liq/1000000).toFixed(1)+'M':liq>=1000?'$'+(liq/1000).toFixed(0)+'k':'$'+liq.toFixed(0);
        lqEl.className = 'sm2-val '+(liq>=25000?'pos':'zero');
      } else { lqEl.textContent='--'; lqEl.className='sm2-val zero'; }
    }
    const spEl = $('sp-'+sym);
    if (spEl) {
      const pnls=[d.structural_total_pnl_bp,d.convex_total_pnl_bp,d.compression_total_pnl_bp,
                   d.obi_total_pnl_bp,d.afe_total_pnl_bp,d.pce_total_pnl_bp,d.bracket_total_pnl_bp];
      const tot = pnls.reduce((a,v)=>a+(v||0),0);
      if (Math.abs(tot) > 0.01) {
        spEl.textContent=(tot>=0?'+':'')+tot.toFixed(1)+'bp';
        spEl.className='sm2-val '+(tot>0?'pos':tot<0?'neg':'zero');
      } else { spEl.textContent='--'; spEl.className='sm2-val zero'; }
    }

    // Card active state + imminent glow when readiness >= 75%
    const card = $('sb-'+sym);
    if (card) {
      const _rs = bestReadiness(d);
      const _imm = _rs >= 0.75 && !d.micro_active;
      card.className = 'sym-card' + (d.micro_active ? ' active' : '') + (_imm ? ' imminent' : '');
    }

    // Engine dots
    // LIQ badge — shows liquidation notional building
    const liqBadge=$('badge-'+sym+'-liq'), liqVal=$('badge-val-'+sym+'-liq');
    if (liqBadge) {
      const liqNot=d.liq_notional||0;
      if (d.liq_active||d.micro_active){
        liqBadge.className='eng-badge active';
        if(liqVal)liqVal.textContent='IN TRADE';
      } else if(liqNot>=25000){
        // Threshold met — waiting for price breakout
        liqBadge.className='eng-badge armed';
        const k=liqNot>=1000000?'$'+(liqNot/1000000).toFixed(1)+'M':'$'+(liqNot/1000).toFixed(0)+'k';
        if(liqVal)liqVal.textContent=k+' LIQ';
      } else if(liqNot>=1000){
        liqBadge.className='eng-badge watching';
        if(liqVal)liqVal.textContent='$'+(liqNot/1000).toFixed(1)+'k';
      } else{
        liqBadge.className='eng-badge';
        if(liqVal)liqVal.textContent='--';
      }
    }
    // BRACKET badge — shows range building progress honestly
    const bkBadge=$('badge-'+sym+'-bracket'),bkVal=$('badge-val-'+sym+'-bracket'),bkBar=$('badge-bar-'+sym);
    if (bkBadge) {
      const st=d.bracket_state||''; // IDLE/RANGE_BUILD/WAIT_CONFIRM/ARMED/IN_POSITION/COOLDOWN
      const rp=Math.round((d.bracket_range_pct||0)*100);
      if(d.bracket_active){
        bkBadge.className='eng-badge active';
        const mv=d.bracket_move_bp||0;
        if(bkVal)bkVal.textContent=(mv>=0?'+':'')+mv.toFixed(1)+'bp';
        if(bkBar)bkBar.style.width='100%';
      } else if(rp>0&&rp<100){
        // Actively building range
        bkBadge.className='eng-badge watching';
        if(bkVal)bkVal.textContent='RANGE '+rp+'%';
        if(bkBar)bkBar.style.width=rp+'%';
      } else if(rp===100){
        // Range complete, waiting for liq+perp confirmation
        bkBadge.className='eng-badge watching';
        if(bkVal)bkVal.textContent='WAIT CONFIRM';
        if(bkBar)bkBar.style.width='100%';
      } else{
        bkBadge.className='eng-badge';
        if(bkVal)bkVal.textContent='--';
        if(bkBar)bkBar.style.width='0%';
      }
    }
    // FUNDING WINDOW badge (BTC/ETH only)
    if (sym === 'btcusdt' || sym === 'ethusdt') {
      const fwBadge=$('badge-'+sym+'-fundwin'), fwVal=$('badge-val-'+sym+'-fundwin');
      if (fwBadge) {
        const secs=d.fundwin_secs_to_next||9999, rateBp=d.fundwin_rate_bp||0;
        const mins=Math.floor(secs/60), ss=secs%60;
        const timeStr=mins+'m'+String(ss).padStart(2,'0')+'s';
        if(d.fundwin_active){fwBadge.className='eng-badge active';const mv=d.fundwin_move_bp||0;if(fwVal)fwVal.textContent=(mv>=0?'+':'')+mv.toFixed(1)+'bp';}
        else if(secs<=180&&Math.abs(rateBp)>=1.5){fwBadge.className='eng-badge armed';if(fwVal)fwVal.textContent=timeStr+' '+(rateBp>=0?'+':'')+rateBp.toFixed(1)+'bp';}
        else if(secs<=600){fwBadge.className='eng-badge watching';if(fwVal)fwVal.textContent=timeStr;}
        else{fwBadge.className='eng-badge';if(fwVal)fwVal.textContent=timeStr;}
      }
    }

    // BASIS badge — shows live perp basis value only
    const baBadge=$('badge-'+sym+'-basis'),baVal=$('badge-val-'+sym+'-basis');
    if (baBadge) {
      const basis=d.perp_basis_bp||0;
      if(d.basis_active){
        baBadge.className='eng-badge active';
        const mv=d.basis_move_bp||0;
        if(baVal)baVal.textContent=(mv>=0?'+':'')+mv.toFixed(1)+'bp';
      } else if(Math.abs(basis)<0.1){
        // No perp data yet
        baBadge.className='eng-badge';
        if(baVal)baVal.textContent='--';
      } else if(basis>=5){
        // Basis spike — approaching entry threshold
        baBadge.className='eng-badge armed';
        if(baVal)baVal.textContent='+'+basis.toFixed(1)+'bp';
      } else{
        // Normal basis reading
        baBadge.className='eng-badge';
        if(baVal)baVal.textContent=(basis>=0?'+':'')+basis.toFixed(1)+'bp';
      }
    }

    // Mini P&L + trades
    const symTrades = currentSessionTrades().filter(t => (t.s||'').replace('USDT','').toUpperCase() === short);
    const symPnl = symTrades.reduce((acc, t) => acc + (+t.p||0), 0);
    const miniPnl = $('mini-pnl-'+sym);
    if (miniPnl) { miniPnl.textContent = (symPnl>=0?'+':'')+symPnl.toFixed(2)+'bp'; miniPnl.className = 'sym-pnl '+(symPnl>0?'pos':symPnl<0?'neg':'zero'); }
    set('mini-t-'+sym, symTrades.length || 0);

    // Session hi/lo
    const hiEl = $('sh-'+sym);
    const loEl = $('sl-'+sym);
    if (hiEl) hiEl.textContent = sessionHi[sym] > 0 ? fmtPrice(sessionHi[sym], short) : '--';
    if (loEl) loEl.textContent = (sessionLo[sym] < Infinity && sessionLo[sym] > 0) ? fmtPrice(sessionLo[sym], short) : '--';
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
  updateLivePositions(data);
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

// ── EMERGENCY KILL ─────────────────────────────────────────────────────────
function showKillModal() {
  const m = document.getElementById('kill-modal');
  const s = document.getElementById('kill-status');
  if (s) s.textContent = '';
  if (m) m.classList.add('show');
}

function hideKillModal() {
  const m = document.getElementById('kill-modal');
  if (m) m.classList.remove('show');
}

// Close modal on backdrop click
document.getElementById('kill-modal').addEventListener('click', function(e) {
  if (e.target === this) hideKillModal();
});

// Close on Escape key
document.addEventListener('keydown', function(e) {
  if (e.key === 'Escape') hideKillModal();
});

async function executeKill() {
  const btn = document.querySelector('.kill-confirm');
  const status = document.getElementById('kill-status');
  if (btn) { btn.textContent = 'SENDING...'; btn.disabled = true; }
  if (status) { status.textContent = ''; status.className = 'kill-status'; }

  try {
    const res = await fetch('/api/kill', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: '{}',
      signal: AbortSignal.timeout(8000)
    });
    const data = await res.json();
    if (data.ok) {
      if (status) { status.textContent = '✓ ' + (data.msg || 'All positions flattened'); status.className = 'kill-status ok'; }
      setTimeout(hideKillModal, 1800);
    } else {
      if (status) { status.textContent = '✗ ' + (data.error || 'Unknown error'); status.className = 'kill-status err'; }
    }
  } catch(e) {
    if (status) { status.textContent = '✗ Request failed: ' + e.message; status.className = 'kill-status err'; }
  } finally {
    if (btn) { btn.textContent = 'FLATTEN ALL NOW'; btn.disabled = false; }
  }
}
// ── END EMERGENCY KILL ──────────────────────────────────────────────────────

/* ════════════════════════════════════════════════════════════════════
   ENGINES PANEL — /api/state2 poller + renderer
   ────────────────────────────────────────────────────────────────────
   Renders Phase 1 (eth_btc_leadlag) + the Multi-Day Trio
   (coinbase_premium_mrev, funding_persistence_fade, vol_compression_breakout).

   Strictly additive:
     - Does NOT touch the existing /api/state poller.
     - Does NOT touch the trade log, win/loss accounting, or any
       existing render path.
     - Reads /api/state2 every ENG_POLL_MS and writes to its own DOM
       subtree (#engines-panel) only.

   Polling cadence: 3 s. These engines fire on minute-to-day timescales,
   so 1 Hz polling buys nothing and adds load.
   ════════════════════════════════════════════════════════════════════ */
const ENG_POLL_MS = 3000;
const ENG_BUFFER_HOURS_TARGET = 23;   // multi-day engines need ~24h ring

function _engClampPct(x){ return Math.max(0, Math.min(100, x)); }

function _engBarClass(pct){
  if (pct >= 100) return 'eng-progress-fill fired';
  if (pct >=  75) return 'eng-progress-fill near';
  return 'eng-progress-fill';
}

function _engFmtBp(v, decimals){
  if (v === null || v === undefined || Number.isNaN(+v)) return '--';
  const n = +v;
  const d = (decimals === undefined) ? 2 : decimals;
  return (n >= 0 ? '+' : '') + n.toFixed(d) + 'bp';
}

function _engFmtBufferSpan(ms){
  if (!ms || ms <= 0) return '0m';
  const mins = ms / 60000;
  if (mins < 60) return mins.toFixed(0) + 'm';
  const hrs = mins / 60;
  return hrs.toFixed(1) + 'h';
}

function _engPnlClass(v){
  const n = +v;
  if (Number.isNaN(n) || n === 0) return '';
  return n > 0 ? 'pos' : 'neg';
}

function _engSetStatus(elId, klass, label){
  const el = $(elId);
  if (!el) return;
  el.className = 'eng-status ' + klass;
  el.textContent = label;
}

function _engSetCardClass(cardId, klass){
  const el = $(cardId);
  if (!el) return;
  el.className = 'eng-card ' + klass;
}

function _engUpdatePos(rowId, valId, s){
  const row = $(rowId);
  if (!row) return;
  if (!s.active){ row.style.display = 'none'; return; }
  row.style.display = '';
  const move  = (+s.move_bp || 0).toFixed(1);
  const mfe   = (+s.mfe_bp  || 0).toFixed(1);
  const mae   = (+s.mae_bp  || 0).toFixed(1);
  const sizeR = (+s.size_R  || 0).toFixed(2);
  set(valId, move + 'bp  (mfe ' + mfe + ' / mae ' + mae + ')  ' + sizeR + 'R');
  const v = $(valId);
  if (v) v.className = 'v ' + _engPnlClass(s.move_bp);
}

function _engUpdateStats(prefix, s){
  set(prefix + '-trades', (+s.total_trades || 0));
  const pnl = +s.total_pnl_bp || 0;
  set(prefix + '-pnl', _engFmtBp(pnl, 1));
  const pe = $(prefix + '-pnl');
  if (pe) pe.style.color = pnl > 0 ? 'var(--green)' : pnl < 0 ? 'var(--red)' : '';
  const trades = +s.total_trades || 0;
  set(prefix + '-wr', trades > 0 ? ((+s.win_rate || 0) * 100).toFixed(0) + '%' : '--');
}

/* ── Engine #5: ETH→BTC Lead-Lag (1-3 min scalp) ── */
function renderEthBtcLeadLag(s){
  if (!s) return;
  const eth    = +s.live_eth_window_bp || 0;
  const btc    = +s.live_btc_window_bp || 0;
  const ethThr = +s.eth_lead_threshold_bp || 25;
  const btcThr = +s.btc_lag_threshold_bp  || 12;
  // Progress = ETH lead as % of trigger threshold (positive direction only).
  const ethPct = ethThr > 0 ? _engClampPct((eth / ethThr) * 100) : 0;
  const bar = $('ell-bar');
  if (bar){ bar.className = _engBarClass(ethPct); bar.style.width = ethPct + '%'; }

  set('ell-eth', _engFmtBp(eth));
  const ev = $('ell-eth'); if (ev) ev.className = 'v ' + _engPnlClass(eth);

  const lagOk = Math.abs(btc) <= btcThr;
  set('ell-btc', _engFmtBp(btc) + (lagOk ? '  ✓' : '  ✗'));
  const bv = $('ell-btc'); if (bv) bv.className = 'v ' + (lagOk ? 'mute' : 'neg');

  if      (s.halted) { _engSetStatus('ell-status', 's-halted', 'HALTED'); _engSetCardClass('eng-ell', 'halted'); }
  else if (s.active) { _engSetStatus('ell-status', 's-active', 'IN POS'); _engSetCardClass('eng-ell', 'active'); }
  else               { _engSetStatus('ell-status', 's-armed',  'ARMED');  _engSetCardClass('eng-ell', 'armed');  }

  _engUpdatePos('ell-pos-row', 'ell-pos', s);
  _engUpdateStats('ell', s);
}

/* ── Engine #6: Coinbase Premium Mean-Revert (3-10 day) ── */
function renderCoinbasePremiumMRev(s){
  if (!s) return;
  const live = +s.live_premium_bp || 0;
  const avg  = +s.avg_24h_premium_bp || 0;
  const trig = +s.premium_trigger_bp || -25;
  // Premium is negative when CB < BN. Bar fills as 24h-avg drops toward trigger.
  const denom = Math.abs(trig);
  const num   = Math.max(0, -avg);
  const pct   = denom > 0 ? _engClampPct((num / denom) * 100) : 0;
  const bar = $('cbp-bar');
  if (bar){ bar.className = _engBarClass(pct); bar.style.width = pct + '%'; }

  set('cbp-avg',  _engFmtBp(avg));
  const av = $('cbp-avg'); if (av) av.className = 'v ' + _engPnlClass(-avg);
  set('cbp-live', _engFmtBp(live));
  const lv = $('cbp-live'); if (lv) lv.className = 'v ' + _engPnlClass(-live);

  const samples = +s.buffer_samples || 0;
  const span    = +s.buffer_span_ms || 0;
  set('cbp-buf', samples + ' samples / ' + _engFmtBufferSpan(span));

  const warming = (span / 3.6e6) < ENG_BUFFER_HOURS_TARGET;
  if      (s.halted) { _engSetStatus('cbp-status', 's-halted',  'HALTED');  _engSetCardClass('eng-cbp', 'halted');  }
  else if (s.active) { _engSetStatus('cbp-status', 's-active',  'IN POS');  _engSetCardClass('eng-cbp', 'active');  }
  else if (warming)  { _engSetStatus('cbp-status', 's-warming', 'WARMING'); _engSetCardClass('eng-cbp', 'warming'); }
  else               { _engSetStatus('cbp-status', 's-armed',   'ARMED');   _engSetCardClass('eng-cbp', 'armed');   }

  _engUpdatePos('cbp-pos-row', 'cbp-pos', s);
  _engUpdateStats('cbp', s);
}

/* ── Engine #7: Funding Persistence Fade (3-7 day, perp-data dependent) ── */
function renderFundingPersistenceFade(s){
  if (!s) return;
  const now  = +s.funding_rate_now_bp || 0;
  const avg  = +s.avg_24h_funding_bp || 0;
  const trig = +s.funding_trigger_bp || -10;
  const denom = Math.abs(trig);
  const num   = Math.max(0, -avg);
  const pct   = denom > 0 ? _engClampPct((num / denom) * 100) : 0;
  const bar = $('fpf-bar');
  if (bar){ bar.className = _engBarClass(pct); bar.style.width = pct + '%'; }

  set('fpf-avg', _engFmtBp(avg));
  const av = $('fpf-avg'); if (av) av.className = 'v ' + _engPnlClass(-avg);
  set('fpf-now', _engFmtBp(now));
  const nv = $('fpf-now'); if (nv) nv.className = 'v ' + _engPnlClass(-now);

  const samples = +s.buffer_samples || 0;
  const span    = +s.buffer_span_ms || 0;
  set('fpf-buf', samples + ' samples / ' + _engFmtBufferSpan(span));

  // Starved = perp data dead (PerpFeed reports funding_rate(SYM_BTC)==0 forever).
  // After ~5 samples with strict 0.0 we're confident the WS data plane is blocked.
  const starved = (now === 0 && avg === 0 && samples > 5);
  if      (s.halted)  { _engSetStatus('fpf-status', 's-halted',  'HALTED');  _engSetCardClass('eng-fpf', 'halted');  }
  else if (s.active)  { _engSetStatus('fpf-status', 's-active',  'IN POS');  _engSetCardClass('eng-fpf', 'active');  }
  else if (starved)   { _engSetStatus('fpf-status', 's-starved', 'NO DATA'); _engSetCardClass('eng-fpf', 'starved'); }
  else if ((span / 3.6e6) < ENG_BUFFER_HOURS_TARGET) {
                        _engSetStatus('fpf-status', 's-warming', 'WARMING'); _engSetCardClass('eng-fpf', 'warming'); }
  else                { _engSetStatus('fpf-status', 's-armed',   'ARMED');   _engSetCardClass('eng-fpf', 'armed');   }

  _engUpdatePos('fpf-pos-row', 'fpf-pos', s);
  _engUpdateStats('fpf', s);
}

/* ── Engine #8: Vol Compression Breakout (8-72h) ── */
function renderVolCompressionBreakout(s){
  if (!s) return;
  const ratio = +s.vol_ratio || 0;
  const thr   = +s.compression_ratio_threshold || 0.5;
  // Lower ratio = more compressed. Bar fills as we approach the threshold.
  let pct = 0;
  if (ratio > 0 && thr > 0 && thr < 1){
    pct = _engClampPct(((1 - ratio) / (1 - thr)) * 100);
  }
  const bar = $('vcb-bar');
  if (bar){ bar.className = _engBarClass(pct); bar.style.width = pct + '%'; }

  set('vcb-ratio', ratio > 0 ? ratio.toFixed(3) : '--');
  const rv = $('vcb-ratio');
  if (rv) rv.className = 'v ' + (ratio > 0 && ratio <= thr ? 'pos' : '');

  const don = +s.donchian_24h_high || 0;
  set('vcb-don', don > 0 ? '$' + don.toFixed(2) : '--');

  const samples = +s.buffer_samples || 0;
  const span    = +s.buffer_span_ms || 0;
  set('vcb-buf', samples + ' samples / ' + _engFmtBufferSpan(span));

  if      (s.halted) { _engSetStatus('vcb-status', 's-halted',  'HALTED');  _engSetCardClass('eng-vcb', 'halted');  }
  else if (s.active) { _engSetStatus('vcb-status', 's-active',  'IN POS');  _engSetCardClass('eng-vcb', 'active');  }
  else if ((span / 3.6e6) < ENG_BUFFER_HOURS_TARGET) {
                       _engSetStatus('vcb-status', 's-warming', 'WARMING'); _engSetCardClass('eng-vcb', 'warming'); }
  else               { _engSetStatus('vcb-status', 's-armed',   'ARMED');   _engSetCardClass('eng-vcb', 'armed');   }

  _engUpdatePos('vcb-pos-row', 'vcb-pos', s);
  _engUpdateStats('vcb', s);
}

/* ── Engine #9: Range Mean Reversion (BTC + ETH, 30-min BB + RSI(14)) ──
   Per-symbol cards. data.range_mean_reversion is an array (BTC, ETH).
   Bar fills as RSI drops toward RSI_ENTRY_MAX (30) — the closer to
   oversold extreme, the closer to firing. */
function renderRangeMeanReversionOne(s, prefix, cardId){
  if (!s) return;
  const rsi    = +s.rsi || 50;
  const rsiMax = +s.rsi_entry_max || 30;
  // Progress = how close RSI is to oversold extreme. 0% at RSI=100, 100% at RSI<=rsiMax.
  let pct = 0;
  if (rsi < 100) pct = _engClampPct(((100 - rsi) / (100 - rsiMax)) * 100);
  const bar = $(prefix + '-bar');
  if (bar){ bar.className = _engBarClass(pct); bar.style.width = pct + '%'; }

  // "Price vs band" — show position as band-relative ratio when ready.
  const px    = +s.price || 0;
  const lower = +s.bb_lower || 0;
  const upper = +s.bb_upper || 0;
  const bandEl = $(prefix + '-band');
  if (bandEl){
    if (s.bb_ready && upper > lower){
      // Position 0..100: 0=at lower, 50=at mean, 100=at upper.
      const rel = ((px - lower) / (upper - lower)) * 100;
      const relTxt = isFinite(rel) ? rel.toFixed(0) + '%' : '--';
      bandEl.textContent = '$' + px.toFixed(2) + ' (band ' + relTxt + ')';
      bandEl.className = 'v ' + (rel <= 5 ? 'pos' : rel >= 95 ? 'neg' : 'mute');
    } else {
      bandEl.textContent = px > 0 ? '$' + px.toFixed(2) + ' (warming)' : '--';
      bandEl.className = 'v mute';
    }
  }

  // RSI(14)
  const rsiEl = $(prefix + '-rsi');
  if (rsiEl){
    if (s.bb_ready){
      rsiEl.textContent = rsi.toFixed(1);
      rsiEl.className = 'v ' + (rsi <= rsiMax ? 'pos' : rsi >= (+s.rsi_exit_min || 60) ? 'neg' : '');
    } else {
      rsiEl.textContent = '--';
      rsiEl.className   = 'v mute';
    }
  }

  // Vol frac (σ/μ) — render as bp; band gates [vol_frac_min, vol_frac_max].
  const vol  = +s.vol_frac || 0;
  const vmin = +s.vol_frac_min || 0.0008;
  const vmax = +s.vol_frac_max || 0.0120;
  const volEl = $(prefix + '-vol');
  if (volEl){
    if (vol > 0){
      const inBand = vol >= vmin && vol <= vmax;
      volEl.textContent = (vol * 10000).toFixed(1) + 'bp ' + (inBand ? '✓' : '✗');
      volEl.className = 'v ' + (inBand ? 'pos' : 'neg');
    } else {
      volEl.textContent = '--';
      volEl.className   = 'v mute';
    }
  }

  // Buffer
  const samples = +s.buffer_samples || 0;
  const span    = +s.buffer_span_ms || 0;
  set(prefix + '-buf', samples + ' samples / ' + _engFmtBufferSpan(span));

  // Status — RMR has a hard "ready" flag (bb_ready) that cleanly separates
  // warming from armed; no time-based heuristic needed.
  const warming = !s.bb_ready;
  if      (s.halted) { _engSetStatus(prefix + '-status', 's-halted',  'HALTED');  _engSetCardClass(cardId, 'halted');  }
  else if (s.active) { _engSetStatus(prefix + '-status', 's-active',  'IN POS');  _engSetCardClass(cardId, 'active');  }
  else if (warming)  { _engSetStatus(prefix + '-status', 's-warming', 'WARMING'); _engSetCardClass(cardId, 'warming'); }
  else               { _engSetStatus(prefix + '-status', 's-armed',   'ARMED');   _engSetCardClass(cardId, 'armed');   }

  _engUpdatePos(prefix + '-pos-row', prefix + '-pos', s);
  _engUpdateStats(prefix, s);
}

function renderRangeMeanReversion(arr){
  if (!Array.isArray(arr)) return;
  // Order matches main.cpp PAPER_SYMBOL_IDS = { SYM_BTC, SYM_ETH }.
  if (arr[0]) renderRangeMeanReversionOne(arr[0], 'rmr-btc', 'eng-rmr-btc');
  if (arr[1]) renderRangeMeanReversionOne(arr[1], 'rmr-eth', 'eng-rmr-eth');
}

/* ── Patch FUND/BASIS pills on the per-symbol cards from /api/state2 ──
   The legacy /api/state path leaves d.perp_funding_rate / d.perp_basis_bp
   at zero on the new VPS, so the symbol-card 'FUND' and 'BASIS' pills
   stay '--'. /api/state2 carries live values via funding_window[i] /
   basis_momentum[i]. Patch them in here every poll cycle.

   Strictly additive: only overwrites pills that are still '--' or that
   we've stamped here previously (tracked via dataset.s2). The legacy
   renderer's writes still take precedence the moment it fills a pill,
   because dataset.s2 is only set when the patcher itself fills it. */
function _patchSymbolFundBasisFromState2(data){
  const symPairs = [
    [0, 'btc'],
    [1, 'eth'],
  ];
  for (const pair of symPairs){
    const idx = pair[0];
    const sym = pair[1];
    // FUND pill — funding_window[idx].current_rate (fractional).
    const fwArr = Array.isArray(data.funding_window) ? data.funding_window : null;
    if (fwArr && fwArr[idx] && typeof fwArr[idx].current_rate === 'number'){
      const fr = fwArr[idx].current_rate;
      const fnEl = $('fn-' + sym);
      if (fnEl && (fnEl.textContent === '--' || fnEl.dataset.s2 === '1')){
        if (fr !== 0){
          const frBp = (fr * 10000).toFixed(2);
          fnEl.textContent = (fr >= 0 ? '+' : '') + frBp + 'bp';
          fnEl.className   = 'sm2-val ' + (fr > 0.0003 ? 'neg' : fr < -0.0002 ? 'pos' : 'zero');
          fnEl.dataset.s2  = '1';
        }
      }
    }

    // BASIS pill — basis_momentum[idx].basis_now (already in bp).
    const bmArr = Array.isArray(data.basis_momentum) ? data.basis_momentum : null;
    if (bmArr && bmArr[idx] && typeof bmArr[idx].basis_now === 'number'){
      const basis = bmArr[idx].basis_now;
      const bsEl = $('bs-' + sym);
      if (bsEl && (bsEl.textContent === '--' || bsEl.dataset.s2 === '1')){
        if (Math.abs(basis) > 0.05){
          bsEl.textContent = (basis >= 0 ? '+' : '') + basis.toFixed(1) + 'bp';
          bsEl.className   = 'sm2-val ' + (basis > 5 ? 'neg' : basis < -5 ? 'pos' : 'zero');
          bsEl.dataset.s2  = '1';
        }
      }
    }
  }
}

async function pollEngines(){
  let res;
  try {
    res = await fetch('/api/state2', { cache:'no-store', signal:AbortSignal.timeout(4000) });
  } catch(e){ return; }
  if (!res.ok) return;
  let data;
  try { data = await res.json(); } catch(e){ return; }
  renderEthBtcLeadLag         (data.eth_btc_leadlag);
  renderCoinbasePremiumMRev   (data.coinbase_premium_mrev);
  renderFundingPersistenceFade(data.funding_persistence_fade);
  renderVolCompressionBreakout(data.vol_compression_breakout);
  renderRangeMeanReversion    (data.range_mean_reversion);
  _patchSymbolFundBasisFromState2(data);
}

pollEngines();
setInterval(pollEngines, ENG_POLL_MS);
