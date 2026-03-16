//  CHIMERA app.js 
// Bell fix: only fire audio/flash for trades that arrive AFTER this page load.

const STORAGE_KEY = 'chimera_trades_v3';
const BOOT_TS = Date.now();

// All symbols in display order  must match SymbolIndex.hpp
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
let latestServerTrades = [];
let audioCtx = null;
let audioUnlocked = false;
let lastPrices = {};
SYMBOLS.forEach(s => lastPrices[s.short.toLowerCase()] = 0);
let wins = 0, losses = 0;
let uptimeStart = null;

//  AUDIO 
function unlockAudio() {
  if (audioUnlocked) return;
  try {
    audioCtx = new (window.AudioContext || window.webkitAudioContext)();
    const buf = audioCtx.createBuffer(1, 1, 22050);
    const src = audioCtx.createBufferSource();
    src.buffer = buf; src.connect(audioCtx.destination); src.start(0);
    audioUnlocked = true;
    const btn = document.getElementById('audio-unlock');
    if (btn) { btn.textContent = ' MUTED OFF'; btn.style.background = 'rgba(0,230,118,.1)'; btn.style.color = 'var(--green)'; btn.style.borderColor = 'var(--green)'; }
  } catch(e) { console.warn('Audio unlock failed:', e); }
}

function playWin() {
  if (!audioUnlocked || !audioCtx) return;
  try {
    const now = audioCtx.currentTime;
    [[0, 880, 1040], [0.22, 1100, 1320]].forEach(([t, f1, f2]) => {
      [f1, f2].forEach((freq, i) => {
        const osc = audioCtx.createOscillator(), gain = audioCtx.createGain(), comp = audioCtx.createDynamicsCompressor();
        comp.threshold.value = -3; comp.ratio.value = 4; comp.attack.value = 0.003; comp.release.value = 0.1;
        osc.connect(gain); gain.connect(comp); comp.connect(audioCtx.destination);
        osc.type = 'sine'; osc.frequency.setValueAtTime(freq, now + t);
        gain.gain.setValueAtTime(0, now + t);
        gain.gain.linearRampToValueAtTime(i === 0 ? 1.8 : 0.9, now + t + 0.008); // louder: 1.21.8
        gain.gain.exponentialRampToValueAtTime(0.3, now + t + 0.1);
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
  el.textContent = ` ${sym}  +${(+pnl).toFixed(2)}bp`;
  el.classList.add('show');
  setTimeout(() => el.classList.remove('show'), 2200);
}

//  STORAGE 
function loadTrades() {
  try { const raw = localStorage.getItem(STORAGE_KEY); localTrades = raw ? JSON.parse(raw) : []; }
  catch(e) { localTrades = []; }
}
function saveTrades() {
  try { localStorage.setItem(STORAGE_KEY, JSON.stringify(localTrades.slice(0, 200))); } catch(e) {}
}
window.clearTrades = function() {
  localTrades = []; latestServerTrades = []; wins = 0; losses = 0;
  localStorage.removeItem(STORAGE_KEY);
  renderTradeLog(); updateWinRate();
};

function tradeSource() {
  return latestServerTrades && latestServerTrades.length ? latestServerTrades : localTrades;
}

function persistentTrades() {
  return tradeSource().filter(t => t.s !== 'SESSION' && t.e !== 'START');
}

// Returns only trades from the most recent engine session (after last SESSION START marker)
function currentSessionTrades() {
  const source = tradeSource();
  const allNonSession = [];
  for (const t of source) {
    if (t.s === 'SESSION' || t.e === 'START') break; // stop at session boundary
    allNonSession.push(t);
  }
  return allNonSession;
}

//  TRADE MERGE 
function mergeTrades(serverLog, isBootLoad) {
  latestServerTrades = serverLog ? serverLog.slice(0, 200) : [];
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
        // making every trade appear 13 hours old  bell never rings
        const tsStr = tr.t
          ? (tr.t.length < 12
              ? new Date().toISOString().slice(0,10) + 'T' + tr.t + 'Z'
              : (tr.t.endsWith('Z') || tr.t.includes('+') ? tr.t : tr.t + 'Z'))
          : null;
        const tradeAge = tsStr ? (Date.now() - new Date(tsStr).getTime()) : 99999;
        const isFresh = tradeAge < 60000; // widened 30s60s: handles slow polls and slightly stale server timestamps
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

//  HELPERS 
const $ = id => document.getElementById(id);
const set = (id, val) => { const el = $(id); if (el) el.textContent = val; };
const fmtPnl = v => (v >= 0 ? '+' : '') + (+v).toFixed(2) + 'bp';
const pnlCls = (base, v) => base + ' ' + (+v > 0 ? 'pos' : +v < 0 ? 'neg' : '');
const fmtClockFromMs = ts => ts ? new Date(ts).toISOString().substring(11, 16) : '--';

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
    const trades = currentSessionTrades().filter(t => (t.s || '').replace('USDT', '').toUpperCase() === short).slice(0, 8);
    if (!trades.length) { el.className = 'trades-empty'; el.innerHTML = 'No trades yet'; return; }
    el.className = 'sym-trades-body';
    el.innerHTML = trades.map(tr => {
      const pnl = +tr.p || 0, isWin = pnl >= 0, usd = bpToUsd(pnl);
      const rc = reasonClass(tr.why || tr.reason || '');
      const en = tr.en ? fmtPrice(tr.en, short) : '--';
      const ex = tr.ex ? fmtPrice(tr.ex, short) : '--';
      const why = normalizeReason(tr.why || tr.reason || '?');
      const time = tr.t ? (tr.t.length > 8 ? tr.t.substring(11, 19) : tr.t) : '--';
      return `<div class="trade-row ${isWin?'win':'loss'}">
        <span class="tr-tag ${isWin?'win':'loss'}">${isWin?'WIN':'LOSS'}</span>
        <span class="tr-pnl ${isWin?'pos':'neg'}">${fmtPnl(pnl)}</span>
        <span class="tr-usd ${isWin?'pos':'neg'}">${fmtUsd(usd)}</span>
        <span class="tr-eng">${tr.e||'--'}</span>
        <span class="tr-val">${en}→${ex}</span>
        <span class="tr-val">${fmtHold(tr.hold)}</span>
        <span class="tr-badge ${rc}">${why}</span>
        <span class="tr-time">${time}</span>
      </div>`;
    }).join('');
  });
}

function updateWinRate() {
  const t = wins + losses;
  const wr = t > 0 ? (wins / t * 100).toFixed(0) + '%' : '--%';
  set('ts-wr', wr);
  const tswr = $('ts-wr');
  if (tswr) tswr.className = 'tl-stat-val ' + (t > 0 ? (wins >= losses ? 'pos' : 'neg') : '');
}

function updateUptime() {
  if (!uptimeStart) return;
  const s = Math.floor((Date.now() - uptimeStart) / 1000);
  const h = Math.floor(s / 3600), m = Math.floor((s % 3600) / 60), sec = s % 60;
  const fmt = `${String(h).padStart(2,'0')}:${String(m).padStart(2,'0')}:${String(sec).padStart(2,'0')}`;
  set('tb-uptime', fmt); set('st-uptime', fmt);
}

function renderTradeLog() {
  let totalPnl = 0, winPnl = 0, lossPnl = 0, winCount = 0, lossCount = 0;
  currentSessionTrades().forEach(t => {
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
  if (pnlEl) { pnlEl.textContent = fmtPnl(totalPnl); pnlEl.className = 'tl-stat-val ' + (totalPnl >= 0 ? 'pos' : 'neg'); }
  const usdEl = $('ts-usd');
  if (usdEl) { usdEl.textContent = fmtUsd(bpToUsd(totalPnl)); usdEl.className = 'tl-stat-val ' + (totalPnl >= 0 ? 'pos' : 'neg'); }
  const wrEl = $('ts-wr');
  if (wrEl) { wrEl.textContent = wr !== null ? (wr*100).toFixed(0)+'%' : '--%'; wrEl.className = 'tl-stat-val ' + (wr !== null ? (wr >= 0.5 ? 'pos' : 'neg') : ''); }
  const awEl = $('ts-avgwin'); if (awEl) awEl.textContent = avgWin !== null ? '+' + avgWin.toFixed(2) + 'bp' : '--';
  const alEl = $('ts-avgloss'); if (alEl) alEl.textContent = avgLoss !== null ? avgLoss.toFixed(2) + 'bp' : '--';
  const expEl = $('ts-exp');
  if (expEl) { expEl.textContent = exp !== null ? (exp >= 0 ? '+' : '') + exp.toFixed(2) + 'bp' : '--'; expEl.className = 'tl-stat-val ' + (exp !== null ? (exp >= 0 ? 'pos' : 'neg') : ''); }
  // Mirror to trades-panel header (tp- IDs) — avoids duplicate ID issues
  set('tp-wins', winCount); set('tp-losses', lossCount);
  const tpWrEl = $('tp-wr'); if (tpWrEl) { tpWrEl.textContent = wr !== null ? (wr*100).toFixed(0)+'%' : '--%'; tpWrEl.className = 'tp-val ' + (wr !== null ? (wr >= 0.5 ? 'pos' : 'neg') : ''); }
  const tpPnlEl = $('tp-pnl'); if (tpPnlEl) { tpPnlEl.textContent = fmtPnl(totalPnl); tpPnlEl.className = 'tp-val ' + (totalPnl >= 0 ? 'pos' : 'neg'); }
  const tpUsdEl = $('tp-usd'); if (tpUsdEl) { tpUsdEl.textContent = fmtUsd(bpToUsd(totalPnl)); tpUsdEl.className = 'tp-val ' + (totalPnl >= 0 ? 'pos' : 'neg'); }
  const tpAwEl = $('tp-avgwin'); if (tpAwEl) tpAwEl.textContent = avgWin !== null ? '+' + avgWin.toFixed(2) + 'bp' : '--';
  const tpAlEl = $('tp-avgloss'); if (tpAlEl) tpAlEl.textContent = avgLoss !== null ? avgLoss.toFixed(2) + 'bp' : '--';
  const tpExpEl = $('tp-exp'); if (tpExpEl) { tpExpEl.textContent = exp !== null ? (exp >= 0 ? '+' : '') + exp.toFixed(2) + 'bp' : '--'; tpExpEl.className = 'tp-val ' + (exp !== null ? (exp >= 0 ? 'pos' : 'neg') : ''); }

  // Recent trades list — last 10 trades from current session, most recent first
  const rtList = $('recent-trades-list');
  if (rtList) {
    const trades = currentSessionTrades();
    if (!trades.length) {
      rtList.innerHTML = '<div style="padding:10px 12px;color:var(--muted);font-size:11px;font-style:italic">Waiting for first trade...</div>';
    } else {
      const recent = trades.slice(0, 10);
      rtList.innerHTML = recent.map(tr => {
        const p = +tr.p || 0;
        const isWin = p >= 0;
        const sym = (tr.s || '').replace('USDT','').replace('/','');
        const eng = (tr.e || '?').toUpperCase();
        const why = normalizeReason(tr.why || tr.reason || '?');
        const whyCls = why === 'TP' ? 'why-tp' : why === 'SL' ? 'why-sl' : why === 'TRAIL' ? 'why-trail' : '';
        const time = tr.t ? (tr.t.length > 10 ? tr.t.substring(11,16) : tr.t) : '--';
        const hold = tr.hold != null ? fmtHold(tr.hold) : '--';
        const usd = bpToUsd(p);
        return `<div class="rt-row ${isWin ? 'rt-win' : 'rt-loss'}">
          <span class="rt-time">${time}</span>
          <span class="rt-sym">${sym}</span>
          <span class="rt-eng">${eng}</span>
          <span class="rt-why ${whyCls}">${why}</span>
          <span class="rt-pnl ${isWin ? 'pos' : 'neg'}">${isWin?'+':''}${p.toFixed(2)}bp</span>
          <span class="rt-hold">${hold}</span>
        </div>`;
      }).join('');
    }
  }

  // Bottom feed — 10 trade rows with full columns
  const btmRows = $('btm-trade-rows');
  if (btmRows) {
    const trades = currentSessionTrades();
    if (!trades.length) {
      btmRows.innerHTML = '<div style="padding:5px 12px;color:var(--muted);font-size:11px;font-style:italic">No trades yet this session</div>';
    } else {
      btmRows.innerHTML = trades.slice(0, 10).map(tr => {
        const p = +tr.p || 0, isWin = p >= 0;
        const sym = (tr.s || '').replace('USDT','').replace('/','');
        const eng = (tr.e || '?').toUpperCase();
        const why = normalizeReason(tr.why || tr.reason || '?');
        const whyCls = why==='TP'?'why-tp':why==='SL'?'why-sl':why==='TRAIL'?'why-trail':'why-to';
        const time = tr.t ? (tr.t.length > 10 ? tr.t.substring(11,16) : tr.t) : '--';
        const mfe = tr.mfe != null ? '+' + (+tr.mfe).toFixed(1) + 'bp' : '--';
        const mae = tr.mae != null ? (+tr.mae).toFixed(1) + 'bp' : '--';
        const hold = tr.hold != null ? fmtHold(tr.hold) : '--';
        const usd = bpToUsd(p);
        return `<div class="btm-trade-row ${isWin?'btr-win':'btr-loss'}">
          <span class="btr-time">${time}</span>
          <span class="btr-sym">${sym}</span>
          <span class="btr-eng">${eng}</span>
          <span class="btr-why ${whyCls}">${why}</span>
          <span class="btr-pnl ${isWin?'pos':'neg'}">${isWin?'+':''}${p.toFixed(2)}bp</span>
          <span class="btr-usd ${isWin?'pos':'neg'}">${fmtUsd(usd)}</span>
          <span class="btr-mfe">${mfe}</span>
          <span class="btr-mae">${mae}</span>
          <span class="btr-hold">${hold}</span>
        </div>`;
      }).join('');
    }
  }
}


//  RIGHT PANEL TRADE LOG
function renderRpTrades() {
  var list = document.getElementById('rp-trade-list');
  if (!list) return;
  var trades = tradeSource().filter(function(t){ return t.s !== 'SESSION'; });
  if (!trades.length) {
    list.innerHTML = '<div style="padding:10px 12px;color:var(--muted);font-size:11px;font-style:italic">No trades yet</div>';
    return;
  }
  var wins = 0, losses = 0, totalPnl = 0;
  trades.forEach(function(t) {
    var p = +t.p || 0; totalPnl += p;
    if (p >= 0) wins++; else losses++;
  });
  var total = wins + losses;
  var wrEl = document.getElementById('rp-wr');
  var wEl  = document.getElementById('rp-wins');
  var lEl  = document.getElementById('rp-losses');
  var pEl  = document.getElementById('rp-pnl');
  if (wEl) wEl.textContent = wins;
  if (lEl) lEl.textContent = losses;
  if (wrEl) wrEl.textContent = total > 0 ? (wins/total*100).toFixed(0)+'%' : '--%';
  if (pEl) {
    pEl.textContent = (totalPnl >= 0 ? '+' : '') + totalPnl.toFixed(2) + 'bp';
    pEl.style.color = totalPnl >= 0 ? 'var(--green)' : 'var(--red)';
  }
  list.innerHTML = trades.slice(0, 100).map(function(tr) {
    var pnl = +tr.p || 0, isWin = pnl >= 0, usd = bpToUsd(pnl);
    var rc  = reasonClass(tr.why || tr.reason || '');
    var sym = (tr.s || '').replace('USDT','').replace('/','');
    var en  = tr.en ? fmtPrice(tr.en, sym) : '--';
    var ex  = tr.ex ? fmtPrice(tr.ex, sym) : '--';
    var why = normalizeReason(tr.why || tr.reason || '?');
    var time = tr.t ? (tr.t.length > 10 ? tr.t.substring(0,16).replace('T',' ') : tr.t) : '--';
    var mfe  = tr.mfe != null ? '+' + (+tr.mfe).toFixed(2) + 'bp' : '--';
    var mae  = tr.mae != null ? (+tr.mae).toFixed(2) + 'bp' : '--';
    var hold = tr.hold != null ? fmtHold(tr.hold) : '--';
    var winCls = isWin ? 'win' : 'loss';
    var pnlCls = isWin ? 'pos' : 'neg';
    var html  = '<div class="rp-trade-row ' + winCls + '">';
    html += '<div class="rptr-top">';
    html += '<span class="rptr-tag ' + winCls + '">' + (isWin?'WIN':'LOSS') + '</span>';
    html += '<span class="rptr-sym">' + sym + '</span>';
    html += '<span class="rptr-eng">' + (tr.e||'--') + '</span>';
    html += '<span class="rptr-pnl ' + pnlCls + '">' + fmtPnl(pnl) + '</span>';
    html += '<span class="rptr-usd ' + pnlCls + '">' + fmtUsd(usd) + '</span>';
    html += '<span class="rptr-badge ' + rc + '">' + why + '</span>';
    html += '</div>';
    html += '<div class="rptr-mid">';
    html += '<span>Entry: <strong>' + en + '</strong></span>';
    html += '<span>Exit: <strong>' + ex + '</strong></span>';
    html += '<span>Hold: <strong>' + hold + '</strong></span>';
    html += '</div>';
    html += '<div class="rptr-bot">';
    html += '<span class="rptr-mfe">MFE ' + mfe + '</span>';
    html += '<span class="rptr-mae">MAE ' + mae + '</span>';
    html += '<span class="rptr-time">' + time + '</span>';
    html += '</div>';
    html += '</div>';
    return html;
  }).join('');
}

function updateOrderDiagnostics(data) {
  const d = data && data.order_diag ? data.order_diag : null;
  if (!d) return;

  set('od-submitted', d.submitted || 0);
  set('od-filled', d.filled || 0);
  set('od-canceled', d.canceled || 0);
  set('od-rejected', d.rejected || 0);
  set('od-timeouts', d.timeout_cancels || 0);
  set('od-fill-rate', d.fill_rate != null ? d.fill_rate.toFixed(0) + '%' : '--');
  set('od-cancel-rate', d.cancel_rate != null ? d.cancel_rate.toFixed(0) + '%' : '--');

  const hint = $('od-hint');
  if (hint) {
    let msg = 'Waiting for order flow.';
    if ((d.submitted || 0) === 0) {
      msg = 'No orders submitted. Trade frequency is limited by signal gating, not execution.';
    } else if ((d.filled || 0) === 0) {
      msg = 'Signals are posting orders, but none are filling. Increase frequency via fill quality, not looser alpha.';
    } else if ((data.total_trades || 0) === 0) {
      msg = 'Orders are filling, but no positions have exited yet.';
    } else if ((d.timeout_cancels || 0) > (d.filled || 0)) {
      msg = 'Most entry attempts are timing out. Safer frequency gains come from better maker joins or longer holdable edges.';
    } else {
      msg = 'Order flow is live. Focus on profitable follow-through, not raw trade count.';
    }
    hint.textContent = msg;
  }

  const byLayer = $('od-by-layer');
  if (byLayer) {
    const rows = (d.by_layer || []).slice(0, 8);
    if (!rows.length) {
      byLayer.innerHTML = '<div style="color:var(--muted);font-size:11px;font-style:italic">No order activity yet</div>';
    } else {
      byLayer.innerHTML = rows.map(row => `
        <div class="stat-row">
          <span class="sr-label">${row.name}</span>
          <span style="display:flex;gap:8px;font-size:12px;color:var(--muted);font-variant-numeric:tabular-nums">
            <span>S ${row.submitted}</span>
            <span style="color:var(--green)">F ${row.filled}</span>
            <span style="color:var(--yellow)">C ${row.canceled}</span>
            <span>${(row.fill_rate || 0).toFixed(0)}%</span>
          </span>
        </div>
      `).join('');
    }
  }

  const recent = $('od-recent');
  if (recent) {
    const rows = (d.recent || []).slice(0, 10);
    if (!rows.length) {
      recent.innerHTML = '<div style="padding:10px 12px;color:var(--muted);font-size:11px;font-style:italic">No recent order events</div>';
    } else {
      recent.innerHTML = rows.map(ev => {
        const cls = ev.event === 'order_filled' ? 'rt-win' :
                    ev.event === 'order_rejected' ? 'rt-loss' : '';
        const reason = ev.reason || ev.status || '--';
        return `<div class="rt-row ${cls}">
          <span class="rt-time">${fmtClockFromMs(ev.ts_ms)}</span>
          <span class="rt-sym">${(ev.symbol || '').replace('usdt','').toUpperCase()}</span>
          <span class="rt-eng">${ev.layer || '--'}</span>
          <span class="rt-why">${(ev.event || '').replace('order_','').toUpperCase()}</span>
          <span class="rt-pnl ${ev.event === 'order_filled' ? 'pos' : ev.event === 'order_rejected' ? 'neg' : ''}">${reason}</span>
          <span class="rt-hold">${ev.order_type || '--'}</span>
        </div>`;
      }).join('');
    }
  }
}

function updateHistoryPanel() {
  const trades = persistentTrades();
  const total = trades.length;
  const winsN = trades.filter(t => (+t.p || 0) >= 0).length;
  const lossesN = total - winsN;
  const totalPnl = trades.reduce((acc, t) => acc + (+t.p || 0), 0);
  const wr = total > 0 ? (winsN / total * 100) : 0;

  set('hist-count', total);
  set('hist-wins', winsN);
  set('hist-losses', lossesN);
  set('hist-wr', total > 0 ? wr.toFixed(0) + '%' : '--%');
  set('hist-pnl', total > 0 ? fmtPnl(totalPnl) : '+0.00bp');

  const hp = $('hist-pnl');
  if (hp) hp.className = 'tp-val ' + (totalPnl > 0 ? 'pos' : totalPnl < 0 ? 'neg' : '');
  const hw = $('hist-wr');
  if (hw) hw.className = 'tp-val ' + (total > 0 ? (wr >= 50 ? 'pos' : 'neg') : '');

  const insightEl = $('hist-insight');
  if (insightEl) {
    let msg = 'Persistent history is loaded from disk and survives restarts.';
    const recent = trades.slice(0, 2);
    if (recent.length === 2 &&
        recent.every(t => t.s === 'BTC' && t.e === 'OFI' && (t.why || t.reason) === 'TIMEOUT') &&
        recent.every(t => (+t.mfe || 0) <= 0.10)) {
      msg = 'The last 2 completed trades were BTC OFI timeouts with no follow-through. Frequency should come from faster OFI recycling and more ETH/BNB/SOL participation, not repeated dead BTC entries.';
    } else if (recent.length > 0) {
      msg = 'History shows which layers are completing trades across resets. Use the order funnel to separate submitted orders from completed exits.';
    }
    insightEl.textContent = msg;
  }

  const list = $('history-trades-list');
  if (!list) return;
  if (!trades.length) {
    list.innerHTML = '<div style="padding:10px 12px;color:var(--muted);font-size:11px;font-style:italic">No persistent completed trades yet</div>';
    return;
  }

  list.innerHTML = trades.slice(0, 12).map(tr => {
    const p = +tr.p || 0;
    const isWin = p >= 0;
    const sym = (tr.s || '').replace('USDT','').replace('/','');
    const eng = (tr.e || '?').toUpperCase();
    const why = normalizeReason(tr.why || tr.reason || '?');
    const time = tr.t ? (tr.t.length > 10 ? tr.t.substring(5,16).replace('T',' ') : tr.t) : '--';
    return `<div class="rt-row ${isWin ? 'rt-win' : 'rt-loss'}">
      <span class="rt-time">${time}</span>
      <span class="rt-sym">${sym}</span>
      <span class="rt-eng">${eng}</span>
      <span class="rt-why">${why}</span>
      <span class="rt-pnl ${isWin ? 'pos' : 'neg'}">${isWin ? '+' : ''}${p.toFixed(2)}bp</span>
      <span class="rt-hold">${fmtHold(tr.hold)}</span>
    </div>`;
  }).join('');
}

function updateQualityPanel() {
  const trades = persistentTrades();
  const box = $('history-quality-list');
  const insightEl = $('history-quality-insight');
  if (!box || !insightEl) return;

  if (!trades.length) {
    insightEl.textContent = 'Layer quality stats appear once completed trades are persisted.';
    box.innerHTML = '<div style="padding:10px 12px;color:var(--muted);font-size:11px;font-style:italic">No persistent layer stats yet</div>';
    return;
  }

  const byLayer = new Map();
  trades.forEach(tr => {
    const layer = (tr.e || '?').toUpperCase();
    if (!byLayer.has(layer)) {
      byLayer.set(layer, { layer, trades: 0, pnl: 0, mfe: 0, timeout: 0, noFollow: 0 });
    }
    const row = byLayer.get(layer);
    row.trades += 1;
    row.pnl += (+tr.p || 0);
    row.mfe += (+tr.mfe || 0);
    const why = (tr.why || tr.reason || '').toUpperCase();
    if (why === 'TIMEOUT') row.timeout += 1;
    if (why === 'NO_FOLLOW') row.noFollow += 1;
  });

  const rows = [...byLayer.values()]
    .map(r => ({
      ...r,
      avgPnl: r.pnl / Math.max(1, r.trades),
      avgMfe: r.mfe / Math.max(1, r.trades),
      timeoutRate: (r.timeout / Math.max(1, r.trades)) * 100,
      noFollowRate: (r.noFollow / Math.max(1, r.trades)) * 100,
    }))
    .sort((a, b) => b.trades - a.trades || b.avgMfe - a.avgMfe);

  const worst = rows[0];
  if (worst && worst.avgMfe < 2.0 && worst.timeoutRate >= 60) {
    insightEl.textContent = `${worst.layer} is the main quality drag right now: avg MFE ${worst.avgMfe.toFixed(2)}bp, avg PnL ${worst.avgPnl.toFixed(2)}bp, timeout rate ${worst.timeoutRate.toFixed(0)}%.`;
  } else {
    insightEl.textContent = 'Use avg MFE and timeout rate to decide which layers should stay active. A layer that cannot produce >8bp gross often enough should be gated harder.';
  }

  box.innerHTML = rows.slice(0, 6).map(r => {
    const pnlClsName = r.avgPnl > 0 ? 'pos' : r.avgPnl < 0 ? 'neg' : '';
    const mfeClsName = r.avgMfe >= 8 ? 'pos' : r.avgMfe >= 4 ? 'warn' : 'neg';
    return `<div class="rt-row ${r.avgPnl >= 0 ? 'rt-win' : 'rt-loss'}">
      <span class="rt-time">${r.layer}</span>
      <span class="rt-sym">${r.trades}T</span>
      <span class="rt-eng ${pnlClsName}">${r.avgPnl >= 0 ? '+' : ''}${r.avgPnl.toFixed(2)}bp</span>
      <span class="rt-why ${mfeClsName}">${r.avgMfe.toFixed(2)}bp MFE</span>
      <span class="rt-pnl">${r.timeoutRate.toFixed(0)}% TO</span>
      <span class="rt-hold">${r.noFollowRate.toFixed(0)}% NF</span>
    </div>`;
  }).join('');
}

//  TRADE CARDS 
function reasonClass(r) {
  if (!r) return 'timeout';
  const rl = r.toLowerCase();
  if (rl === 'tp') return 'tp';
  if (rl === 'sl' || rl === 'sc') return 'sl';
  if (rl === 'trail' || rl.startsWith('trail')) return 'trail';
  if (rl === 'timeout' || rl === 'to') return 'timeout';
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

function makeRow(tr) {
  var pnl = +tr.p || 0, isWin = pnl >= 0, usd = bpToUsd(pnl);
  var rc  = reasonClass(tr.why || tr.reason || '');
  var sym = (tr.s || '').replace('USDT','').replace('/','');
  var en  = tr.en ? fmtPrice(tr.en, sym) : '--';
  var ex  = tr.ex ? fmtPrice(tr.ex, sym) : '--';
  var why = normalizeReason(tr.why || tr.reason || '?');
  var time = tr.t ? tr.t.substring(0,16).replace('T',' ') : '--';
  var mfe  = tr.mfe != null ? '+' + (+tr.mfe).toFixed(2) + 'bp' : '--';
  var mae  = tr.mae != null ? (+tr.mae).toFixed(2) + 'bp' : '--';
  var hold = tr.hold != null ? fmtHold(tr.hold) : '--';
  var wc = isWin ? 'win' : 'loss', pc = isWin ? 'pos' : 'neg';
  var h = '<div class="tl-row ' + wc + '">';
  h += '<div class="tl-r1"><span class="tl-tag ' + wc + '">' + (isWin?'W':'L') + '</span>';
  h += '<span class="tl-sym">' + sym + '</span><span class="tl-eng">' + (tr.e||'--') + '</span>';
  h += '<span class="tl-pnl ' + pc + '">' + fmtPnl(pnl) + '</span></div>';
  h += '<div class="tl-r2"><span>In:<strong>' + en + '</strong></span>';
  h += '<span>Out:<strong>' + ex + '</strong></span>';
  h += '<span>Hold:<strong>' + hold + '</strong></span></div>';
  h += '<div class="tl-r3"><span class="tl-mfe">MFE ' + mfe + '</span>';
  h += '<span class="tl-mae">MAE ' + mae + '</span>';
  h += '<span class="tl-why ' + rc + '">' + why + '</span>';
  h += '<span class="tl-time">' + time + '</span></div>';
  h += '</div>';
  return h;
}

//  REGIME STATE 
function regimeClass(state) {
  if (!state) return 'rs-neutral';
  const s = state.toUpperCase();
  if (s.includes('BREAKOUT') || s.includes('BURST')) return 'rs-burst';
  if (s.includes('BUILDUP') || s.includes('TREND')) return 'rs-trending';
  if (s.includes('COMPRESSION')) return 'rs-compression';
  if (s.includes('DEAD')) return 'rs-dead';
  if (s.includes('GRIND')) return 'rs-neutral';
  return 'rs-neutral';
}

//  MAIN UPDATE 
let firstPoll = true;
let lastKnownUptimeHours = null;

function updateAll(data) {
  if (!data) return;
  if (!uptimeStart) uptimeStart = Date.now();

  //  Restart detection 
  // Server sends uptime_hours. If it goes backwards (or resets near 0),
  // the server restarted  clear stale localStorage trades from old session.
  const serverUptime = data.uptime_hours || 0;
  if (lastKnownUptimeHours !== null && serverUptime < lastKnownUptimeHours - 0.01) {
    // Server restarted  clear old session trades and silence bell on re-merge
    console.log('[Chimera] Server restart detected (uptime reset). Clearing old session trades.');
    localTrades = [];
    try { localStorage.removeItem(STORAGE_KEY); } catch(e) {}
    uptimeStart = Date.now();
    firstPoll = true;  // treat next trade merge as boot load  no bell for old trades
  }
  lastKnownUptimeHours = serverUptime;

  const updatePrice = (id, val, prev, sym) => {
    const el = $(id); if (!el) return;
    el.textContent = fmtPrice(val, sym);
    el.className = 'sym-px' + (val > prev ? ' up' : val < prev ? ' down' : '');
  };

  // Update all symbol prices dynamically
  SYMBOLS.forEach(({ short, full }) => {
    const key = short.toLowerCase();
    // Backend emits full symbol price keys: btcusdt_price, ethusdt_price etc
    const px = data[full + '_price'] || data[short.toUpperCase() + '_price'] || data[short.toLowerCase() + '_price'] || 0;
    updatePrice('px-' + key, px, lastPrices[key], short);
    lastPrices[key] = px;
  });

//  BOOST MULTIPLIER PANEL 
function updateBoostPanel(data) {
  const engines = [
    { key: 'boost_leadlag',    id: 'leadlag'    },
    { key: 'boost_ll_eth_sol', id: 'll-eth-sol' },
    { key: 'boost_impulse',    id: 'impulse'    },
    { key: 'boost_expand',     id: 'expand'     },
    { key: 'boost_liq',        id: 'liq'        },
    { key: 'boost_fund',       id: 'fund'       },
    { key: 'boost_ngas',       id: 'ngas'       },
    { key: 'boost_eth_lead',   id: 'eth-lead'   },
    { key: 'boost_sol_lead',   id: 'sol-lead'   },
    { key: 'boost_volshock',   id: 'volshock'   },
    { key: 'boost_ofi',        id: 'ofi'        },
    { key: 'boost_sweep',      id: 'sweep'      },
    { key: 'boost_mm',         id: 'mm'         },
  ];
  const MAX_BOOST = 2.5;
  engines.forEach(({ key, id }) => {
    const val = data[key] !== undefined ? +data[key] : 1.0;
    const pct = Math.min(100, ((val - 1.0) / (MAX_BOOST - 1.0)) * 100);
    const isMax = val >= MAX_BOOST - 0.05;
    const barEl = $('bbar-' + id);
    const valEl = $('bval-' + id);
    if (barEl) {
      barEl.style.width = pct + '%';
      barEl.className = 'boost-bar-fill' + (isMax ? ' max' : '');
    }
    if (valEl) {
      valEl.textContent = val.toFixed(2) + 'x';
      valEl.className = 'boost-val' + (isMax ? ' max' : '');
    }
  });

  // Layer Adapt Panel — shows LayerPerformanceTracker state for OFI/SWEEP/MM-PRESSURE
  const la = data.layer_adapt;
  if (la) {
    const ADAPT_LAYERS = [
      { key: 'ofi',   label: 'OFI'         },
      { key: 'sweep', label: 'SWEEP'        },
      { key: 'mm',    label: 'MM-PRESS'     },
    ];
    ADAPT_LAYERS.forEach(({ key, label }) => {
      const d = la[key];
      if (!d) return;
      const trades  = d.trades || 0;
      const pnlEma  = d.pnl_ema || 0;
      const mult    = d.mult || 1.0;
      const active  = trades >= 10;  // MIN_TRADES from LayerPerformanceTracker
      const multEl  = $('la-mult-' + key);
      const tradesEl = $('la-trades-' + key);
      const emaEl   = $('la-ema-' + key);
      const statusEl = $('la-status-' + key);
      if (multEl) {
        multEl.textContent = mult.toFixed(2) + 'x';
        multEl.className = 'la-mult ' + (mult > 1.0 ? 'pos' : mult < 1.0 ? 'neg' : 'muted');
      }
      if (tradesEl) tradesEl.textContent = trades + 'T';
      if (emaEl) {
        emaEl.textContent = (pnlEma >= 0 ? '+' : '') + pnlEma.toFixed(1) + 'bp';
        emaEl.className = 'la-ema ' + (pnlEma >= 0 ? 'pos' : 'neg');
      }
      if (statusEl) {
        statusEl.textContent = active ? 'ACTIVE' : 'WARMUP';
        statusEl.className = 'la-status ' + (active ? 'la-active' : 'la-warmup');
      }
    });
  }
}

  // Top bar
  const pnl = data.pnl || 0;
  const pnlEl = $('tb-pnl');
  if (pnlEl) { pnlEl.textContent = fmtPnl(pnl); pnlEl.className = 'tb-val ' + (pnl > 0 ? 'pos' : pnl < 0 ? 'neg' : ''); }
  const equity = $('tb-equity');
  if (equity) equity.textContent = '$' + (10000 + pnl).toLocaleString('en-US', {minimumFractionDigits:2,maximumFractionDigits:2});
  set('tb-trades', data.total_trades || 0);
  set('tb-positions', data.open_positions || 0);
  const lat = data.latency_p95 || 0;
  // WS Delay p95: Binance WS gateway delay. Same AWS Tokyo: our net ~0.5ms, Binance pipeline ~35ms = ~36ms total. Normal.
  // Green <25ms, amber 25-50ms, red >50ms = genuine feed problem.
  const latColour = lat <= 0 ? '' : lat < 25 ? 'pos' : lat < 50 ? 'warn' : 'neg';
  const latEl = $('tb-latency');
  if (latEl) {
    latEl.textContent = lat > 0 ? lat.toFixed(1) + 'ms' : '--ms';
    latEl.className = 'tb-val accent ' + latColour;
  }

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
  const stLatEl = $('st-lat');
  if (stLatEl) {
    stLatEl.textContent = lat > 0 ? lat.toFixed(1) + 'ms' : '--ms';
    stLatEl.className = stLatEl.className.replace(/\b(pos|warn|neg)\b/g,'').trim() + ' ' + latColour;
  }

  // Boost multiplier panel
  updateBoostPanel(data);
  updateOrderDiagnostics(data);
  updateHistoryPanel();
  updateQualityPanel();

  // Exit breakdown
  if (data.session) {
    const s = data.session;
    set('st-tp', s.tp_exits || 0); set('st-sl', s.sl_exits || 0);
    set('st-trail', s.trail_exits || 0); set('st-timeout', s.timeout_exits || 0);
    // Mirror to trades-panel header
    set('tp-tp', s.tp_exits || 0); set('tp-sl', s.sl_exits || 0);
    set('tp-trail', s.trail_exits || 0); set('tp-timeout', s.timeout_exits || 0);
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

  // Per-symbol blocks  dynamic over all SYMBOLS
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
    if (rmEl) { rmEl.textContent = '' + mult.toFixed(2); rmEl.className = 'regime-mult ' + (mult > 1.1 ? 'hi' : mult < 0.9 ? 'lo' : ''); }

    // Mini summary — symbol total P&L + trade count from localStorage
    const miniPnl = $(`mini-pnl-${sym}`);
    const symTrades = currentSessionTrades().filter(t => (t.s || '').replace('USDT','').toUpperCase() === short);
    const symPnl = symTrades.reduce((acc, t) => acc + (+t.p || 0), 0);
    const symT   = symTrades.length;
    if (miniPnl) { miniPnl.textContent = (symPnl >= 0 ? '+' : '') + symPnl.toFixed(2) + 'bp'; miniPnl.className = 'sym-pnl-badge ' + (symPnl > 0 ? 'pos' : symPnl < 0 ? 'neg' : 'zero'); }
    set(`mini-t-${sym}`, symT ? symT + 'T' : '0T');
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

//  POLL LOOP 
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

  // Error banner  always visible with full detail
  const banner = $('poll-error');
  if (banner) banner.classList.add('show');
  const msg = $('poll-error-msg');
  if (msg) msg.textContent = reason;
  const cnt = $('poll-error-count');
  if (cnt) cnt.textContent = 'ERR' + pollErrors;
  const etime = $('poll-error-time');
  if (etime) etime.textContent = 'last ok: ' + fmtAgo(lastPollOk);

  // Last-poll indicator  goes red when stale
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
    const reason = e.name === 'TimeoutError' ? 'Fetch timeout (>4s)  backend hung?' :
                   e.name === 'TypeError'    ? 'Network error  backend down or unreachable' :
                   'Fetch failed: ' + e.message;
    setPollError(reason);
    return;
  }

  if (!res.ok) {
    setPollError('HTTP ' + res.status + ' ' + res.statusText + '  backend returned error');
    return;
  }

  let data;
  try {
    data = await res.json();
  } catch(e) {
    // Got a response but it wasn't valid JSON  backend probably crashed mid-write
    let raw = '';
    try { raw = await res.text(); } catch(_) {}
    setPollError('JSON parse error  backend may have crashed. Preview: ' + raw.slice(0, 80));
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

//  INIT 
loadTrades(); renderTradeLog(); updateWinRate();
wins = 0; losses = 0;
currentSessionTrades().forEach(t => { if (+t.p > 0) wins++; else if (+t.p < 0) losses++; });
updateWinRate();
poll();
setInterval(poll, 1000);
setInterval(updateUptime, 1000);

//  COLLAPSIBLE SYM-BLOCKS 
function toggleSym(sl, event) {
  if (event) event.stopPropagation();
  const block = document.getElementById('sb-' + sl);
  if (!block) return;
  block.classList.toggle('expanded');
}
// Legacy alias
function toggleBlock(sl, event) { toggleSym(sl, event); }

// Auto-expand a block when it has an active trade
function autoExpandIfActive(sl, isActive) {
  const block = document.getElementById('sb-' + sl);
  if (!block) return;
  if (isActive && !block.classList.contains('expanded')) {
    block.classList.add('expanded');
  }
}

function switchTab(name) {
  ['regime','boost','engine','signal','trades'].forEach(t => {
    const tab = document.getElementById('tab-' + t);
    const content = document.getElementById('tc-' + t);
    if (tab) tab.classList.toggle('active', t === name);
    if (content) content.style.display = t === name ? 'flex' : 'none';
  });
  if (name === 'trades') renderRpTrades();
}
