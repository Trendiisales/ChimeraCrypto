// ============================================================================
// Chimera Trend Engine — app.js
// Uses existing index.html element IDs. Replaces microstructure bindings
// with TrendEngine (H1 EMA crossover) data.
// ============================================================================
'use strict';

const SYMS = ['btc','eth','sol','bnb','avax','link','xrp','doge'];
let g_poll_err = 0;
const g_prev_prices = {};  // track previous price per symbol for direction

const $ = id => document.getElementById(id);

function fmt(v, dp) {
    if (!v || !isFinite(v)) return '--';
    return v.toFixed(dp !== undefined ? dp : (v < 10 ? 5 : 2));
}

function setTxt(id, v) { const e=$(id); if(e) e.textContent=v; }

// Map TrendEngine position to the old card element IDs
function updateCard(sym, pos, price) {
    const s = sym; // e.g. 'btc'

    // Readiness bar
    const bars = pos.bars || 0;
    const pct  = Math.min(100, (bars / 50) * 100);
    const fill = $('bar-fill-'+s);
    if (fill) {
        fill.style.width = pct + '%';
        fill.style.background = pos.ready
            ? 'var(--green)'
            : pct > 50 ? 'var(--yellow)' : 'var(--border2)';
    }
    setTxt('rdy-'+s, pos.ready ? 'READY' : Math.floor(pct)+'%');

    // Price with direction colouring
    const pxEl = $('px-'+s);
    if (pxEl) {
        const p = price || pos.entry || 0;
        pxEl.textContent = p > 0
            ? p.toLocaleString('en',{maximumFractionDigits: p<10?5:2})
            : '--';
        const prev = g_prev_prices[s] || 0;
        let dir = '';
        if (pos.active) { dir = pos.side==='LONG' ? ' up' : ' down'; }
        else if (p > 0 && prev > 0) { dir = p > prev ? ' up' : p < prev ? ' down' : ''; }
        pxEl.className = 'sym-price' + dir;
        g_prev_prices[s] = p;
    }

    // Regime pill — repurposed for EMA trend bias
    const regEl = $('reg-'+s);
    if (regEl) {
        if (!pos.ready) {
            regEl.textContent = 'WARMING';
            regEl.className = 'sym-regime-pill rp-neutral';
        } else if (pos.active) {
            regEl.textContent = pos.side;
            regEl.className = 'sym-regime-pill ' + (pos.side==='LONG' ? 'rp-breakout' : 'rp-shock');
        } else {
            const up = pos.ema9 > pos.ema50;
            const sep = pos.ema50 > 0 ? (Math.abs(pos.ema9-pos.ema50)/pos.ema50*100).toFixed(2)+'%' : '';
            regEl.textContent = (up ? '↑ BULL' : '↓ BEAR') + ' ' + sep;
            regEl.className = 'sym-regime-pill ' + (up ? 'rp-buildup' : 'rp-grind');
        }
    }

    // VOL slot → EMA9
    const vrEl = $('vr-'+s);
    if (vrEl) vrEl.textContent = pos.ema9 > 0 ? fmt(pos.ema9) : '--';

    // DISP slot → EMA50
    const dpEl = $('dp-'+s);
    if (dpEl) dpEl.textContent = pos.ema50 > 0 ? fmt(pos.ema50) : '--';

    // CAP slot → ATR
    const capEl = $('cap-'+s);
    if (capEl) capEl.textContent = pos.atr > 0 ? fmt(pos.atr) : '--';

    // T slot → bar count
    setTxt('mini-t-'+s, bars);

    // FUND slot → Entry price
    const fnEl = $('fn-'+s);
    if (fnEl) {
        fnEl.textContent = pos.active && pos.entry > 0 ? fmt(pos.entry) : '--';
        fnEl.className = 'sm2-val ' + (pos.active ? 'pos' : 'zero');
    }

    // BASIS slot → SL price
    const bsEl = $('bs-'+s);
    if (bsEl) {
        bsEl.textContent = pos.active && pos.sl > 0 ? fmt(pos.sl) : '--';
        bsEl.className = 'sm2-val ' + (pos.active ? 'neg' : 'zero');
    }

    // LIQ slot → MFE
    const lqEl = $('lq-'+s);
    if (lqEl) {
        lqEl.textContent = pos.active && pos.mfe > 0 ? '+'+pos.mfe.toFixed(4) : '--';
        lqEl.className = 'sm2-val ' + (pos.mfe > 0 ? 'pos' : 'zero');
    }

    // SES PNL → trail status
    const spEl = $('sp-'+s);
    if (spEl) {
        spEl.textContent = pos.active ? (pos.trail_armed ? 'TRAIL' : 'HOLD') : '--';
        spEl.className = 'sm2-val ' + (pos.trail_armed ? 'pos' : 'zero');
    }

    // Engine badges — repurposed for trend engine state
    // LIQ badge → EMA crossover signal
    const liqBadge = $('badge-val-'+s+'-liq');
    if (liqBadge) {
        if (!pos.ready) { liqBadge.textContent = 'WARMUP'; liqBadge.style.color = 'var(--muted)'; }
        else if (pos.active) { liqBadge.textContent = pos.side; liqBadge.style.color = pos.side==='LONG'?'var(--green)':'var(--red)'; }
        else { liqBadge.textContent = pos.ema9 > pos.ema50 ? 'BULL' : 'BEAR'; liqBadge.style.color = pos.ema9>pos.ema50?'var(--green)':'var(--red)'; }
    }

    // BRACKET badge → EMA separation %
    const braBadge = $('badge-val-'+s+'-bracket');
    if (braBadge) {
        const sep = pos.ema50 > 0 ? (Math.abs(pos.ema9-pos.ema50)/pos.ema50*100) : 0;
        braBadge.textContent = sep.toFixed(2)+'%';
        braBadge.style.color = sep >= 0.3 ? 'var(--green)' : 'var(--muted)';
        // Fill bar: sep / 2% = 0-100%
        const barEl = $('badge-bar-'+s);
        if (barEl) barEl.style.width = Math.min(100, sep/2*100)+'%';
    }

    // BASIS badge → ATR
    const basBadge = $('badge-val-'+s+'-basis');
    if (basBadge) basBadge.textContent = pos.atr > 0 ? 'ATR '+fmt(pos.atr) : '--';

    // FUND badge → bars / 50 warmup
    const fwBadge = $('badge-val-'+s+'-fundwin');
    if (fwBadge) fwBadge.textContent = bars + '/50';

    // Card active state
    const card = $('sb-'+s);
    if (card) {
        if (pos.active) {
            card.classList.add('active');
            card.style.borderColor = pos.side==='LONG' ? 'var(--green)' : 'var(--red)';
            card.style.boxShadow = pos.side==='LONG'
                ? '0 0 12px rgba(0,230,118,.2)'
                : '0 0 12px rgba(255,61,87,.2)';
        } else {
            card.classList.remove('active');
            card.style.borderColor = '';
            card.style.boxShadow = '';
        }
    }

    // mini-pnl (per-card P&L — use session pnl_pct if available)
    const miniPnl = $('mini-pnl-'+s);
    if (miniPnl && pos.pnl_pct !== undefined) {
        const p = pos.pnl_pct || 0;
        miniPnl.textContent = (p>=0?'+':'')+p.toFixed(3)+'%';
        miniPnl.className = 'sym-pnl ' + (p>0?'pos':p<0?'neg':'zero');
    }
}

// ── Trade log ─────────────────────────────────────────────────────────────────
function renderTradeLog(trades) {
    const el = $('btm-trade-rows');
    if (!el) return;
    if (!trades || trades.length === 0) {
        el.innerHTML = '<tr><td colspan="8" style="text-align:center;color:var(--muted);padding:12px">No trades yet — waiting for H1 bar warmup (~50h)</td></tr>';
        return;
    }
    el.innerHTML = [...trades].reverse().slice(0,60).map(t => {
        const p = t.pnl_pct || 0;
        const pc = p>0?'var(--green)':p<0?'var(--red)':'var(--muted)';
        const sc = t.side==='LONG'?'var(--green)':'var(--red)';
        return `<tr>
          <td style="color:var(--muted);font-size:10px">${t.time||'--'}</td>
          <td style="font-weight:700">${(t.sym||'').toUpperCase()}</td>
          <td style="color:${sc}">${t.side||'--'}</td>
          <td>${t.entry>0?t.entry.toFixed(t.entry<10?5:2):'--'}</td>
          <td>${t.exit>0?t.exit.toFixed(t.exit<10?5:2):'--'}</td>
          <td style="color:${pc}">${p>=0?'+':''}${p.toFixed(3)}%</td>
          <td style="color:var(--muted)">${t.why||'--'}</td>
          <td style="color:var(--muted)">${t.bars_held||'--'}</td>
        </tr>`;
    }).join('');
}

// ── Live positions panel ──────────────────────────────────────────────────────
function renderLivePositions(positions, prices) {
    const el = $('live-positions-list');
    if (!el) return;
    const active = (positions||[]).filter(p=>p.active);
    if (active.length === 0) {
        el.innerHTML = '<div class="live-pos-empty">No open positions — waiting for H1 EMA crossover signal</div>';
        return;
    }
    el.innerHTML = active.map(pos => {
        const price = (prices||{})[pos.sym?.toUpperCase()] || 0;
        const entry = pos.entry || 0;
        const unreal = price > 0 && entry > 0
            ? (pos.side==='LONG' ? (price-entry)/entry*100 : (entry-price)/entry*100)
            : 0;
        const uc = unreal>=0?'var(--green)':'var(--red)';
        return `<div class="live-pos">
          <span style="color:${pos.side==='LONG'?'var(--green)':'var(--red)'};font-weight:700;font-size:12px">${(pos.sym||'').toUpperCase()}</span>
          <span style="color:${pos.side==='LONG'?'var(--green)':'var(--red)'};font-size:11px">${pos.side}</span>
          <span style="color:var(--text);font-size:11px">@ ${entry>0?entry.toFixed(entry<10?5:2):'--'}</span>
          <span style="color:var(--red);font-size:11px">SL ${pos.sl>0?pos.sl.toFixed(pos.sl<10?5:2):'--'}</span>
          <span style="color:${uc};font-size:11px">${unreal>=0?'+':''}${unreal.toFixed(3)}% ${pos.trail_armed?'[TRAIL]':''}</span>
        </div>`;
    }).join('');
}

// ── Session stats panel ───────────────────────────────────────────────────────
function renderSessionStats(d) {
    // Repurpose the session stats grid for trend engine metrics
    const stats = [
        { id:'ss-trades',  label:'TRADES',     val: d.trades || 0 },
        { id:'ss-winrate', label:'WIN RATE',    val: ((d.win_rate||0)*100).toFixed(1)+'%' },
        { id:'ss-pnl',     label:'SESSION P&L', val: ((d.total_pnl_pct||0)>=0?'+':'')+(d.total_pnl_pct||0).toFixed(3)+'%' },
        { id:'ss-active',  label:'ACTIVE POS',  val: (d.positions||[]).filter(p=>p.active).length },
        { id:'ss-ready',   label:'SYMS READY',  val: (d.positions||[]).filter(p=>p.ready).length + '/' + (d.positions||[]).length },
        { id:'ss-warmup',  label:'WARMUP',      val: Math.floor(Math.min(100, ((d.positions||[])[0]?.bars||0)/50*100))+'%' },
    ];
    stats.forEach(s => {
        const el = $(s.id);
        if (el) el.textContent = s.val;
    });
}

// ── Main apply state ──────────────────────────────────────────────────────────
function applyState(d) {
    // Topbar
    const pnl = d.total_pnl_pct || 0;
    const pnlEl = $('tb-pnl');
    if (pnlEl) {
        pnlEl.textContent = (pnl>=0?'+':'')+pnl.toFixed(3)+'%';
        pnlEl.className = 'tb-val ' + (pnl>0?'pos':pnl<0?'neg':'');
    }
    setTxt('tb-trades', d.trades || 0);
    const active_n = (d.positions||[]).filter(p=>p.active).length;
    setTxt('tb-positions', active_n + '/' + (d.positions||[]).length);
    setTxt('build-ver', d.build || '');

    // Shadow badge
    const shadowBadge = $('badge-shadow');
    if (shadowBadge) shadowBadge.style.display = d.shadow ? '' : 'none';

    // Connection dot
    const dot = $('conn-dot');
    if (dot) dot.className = 'dot live';

    // Symbol cards
    const prices = d.prices || {};
    (d.positions || []).forEach((pos, i) => {
        const sym = SYMS[i];
        if (sym) updateCard(sym, pos, prices[sym.toUpperCase()]);
    });

    // Live positions
    renderLivePositions(d.positions, prices);

    // Session stats
    renderSessionStats(d);

    // Trade log
    renderTradeLog(d.trade_log || []);
}

// ── Poll ──────────────────────────────────────────────────────────────────────
async function poll() {
    try {
        const res = await fetch('/api/state', {cache:'no-store', signal:AbortSignal.timeout(4000)});
        if (!res.ok) throw new Error('HTTP '+res.status);
        const d = await res.json();
        g_poll_err = 0;
        applyState(d);
    } catch(e) {
        g_poll_err++;
        const dot = $('conn-dot');
        if (dot) dot.className = 'dot';
    }
    setTimeout(poll, 2000);
}

// ── Kill ──────────────────────────────────────────────────────────────────────
function showKillModal() {
    if (!confirm('Kill all open positions?')) return;
    fetch('/api/kill', {method:'POST'}).catch(()=>{});
}

// ── Init ──────────────────────────────────────────────────────────────────────
document.addEventListener('DOMContentLoaded', poll);
