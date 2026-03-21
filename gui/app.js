// CHIMERA app.js
const STORAGE_KEY = 'chimera_trades_v3';
const SYMBOLS = [
  { short:'BTC',  full:'btcusdt'  },
  { short:'ETH',  full:'ethusdt'  },
  { short:'SOL',  full:'solusdt'  },
  { short:'BNB',  full:'bnbusdt'  },
  { short:'AVAX', full:'avaxusdt' },
  { short:'LINK', full:'linkusdt' },
  { short:'POL',  full:'polusdt'  },
];

// Build symbol cards dynamically
(function buildCards() {
  const grid = document.getElementById('sym-grid');
  if (!grid) return;
  SYMBOLS.forEach(({ short }) => {
    const sym = short.toLowerCase();
    grid.innerHTML += `
      <div class="sym-card" id="sb-${sym}">
        <div class="sym-bar" id="bar-${sym}"></div>
        <div class="sym-body">
          <div class="sym-name">${short}/USDT</div>
          <div class="sym-price" id="px-${sym}">--</div>
          <span class="sym-regime-pill rp-neutral" id="reg-${sym}">NEUTRAL</span>
          <div class="sym-metrics">
            <div class="sm-item"><span class="sm-label">Vol Ratio</span><span class="sm-val" id="vr-${sym}">--</span></div>
            <div class="sm-item"><span class="sm-label">Displacement</span><span class="sm-val" id="dp-${sym}">--</span></div>
            <div class="sm-item"><span class="sm-label">Cap R</span><span class="sm-val accent" id="cap-${sym}">--</span></div>
            <div class="sm-item"><span class="sm-label">Regime Mult</span><span class="sm-val" id="rm-${sym}">--</span></div>
          </div>
          <div class="sym-eng-row">
            <div class="eng-pip" id="ed-${sym}-micro" title="MICRO"></div>
            <div class="eng-pip" id="ed-${sym}-structural" title="STRUCT"></div>
            <div class="eng-pip" id="ed-${sym}-convex" title="CONVEX"></div>
            <div class="eng-pip" id="ed-${sym}-compression" title="COMPR"></div>
            <span class="eng-label" id="edl-${sym}">FLAT</span>
          </div>
          <div class="sym-pnl-row">
            <span class="sym-pnl zero" id="mini-pnl-${sym}">+0.00bp</span>
            <span class="sym-tcount" id="mini-t-${sym}">0T</span>
          </div>
        </div>
      </div>`;
  });
})();

let localTrades = [];
let audioCtx = null, audioUnlocked = false;
let lastPrices = {}; SYMBOLS.forEach(s => lastPrices[s.short.toLowerCase()] = 0);
let wins = 0, losses = 0, uptimeStart = null;

function unlockAudio() {
  if (audioUnlocked) return;
  try {
    audioCtx = new (window.AudioContext || window.webkitAudioContext)();
    const b = audioCtx.createBuffer(1,1,22050), s = audioCtx.createBufferSource();
    s.buffer = b; s.connect(audioCtx.destination); s.start(0);
    audioUnlocked = true;
    const btn = document.getElementById('audio-unlock');
    if (btn) { btn.textContent = 'MUTED OFF'; btn.style.color = 'var(--green)'; }
  } catch(e) {}
}

function playWin() {
  if (!audioUnlocked || !audioCtx) return;
  try {
    const now = audioCtx.currentTime;
    [[0,880,1040],[0.22,1100,1320]].forEach(([t,f1,f2]) => {
      [f1,f2].forEach((freq,i) => {
        const o=audioCtx.createOscillator(), g=audioCtx.createGain();
        o.connect(g); g.connect(audioCtx.destination);
        o.type='sine'; o.frequency.setValueAtTime(freq, now+t);
        g.gain.setValueAtTime(0, now+t);
        g.gain.linearRampToValueAtTime(i===0?1.8:0.9, now+t+0.008);
        g.gain.exponentialRampToValueAtTime(0.001, now+t+1.4);
        o.start(now+t); o.stop(now+t+1.5);
      });
    });
  } catch(e) {}
}

function playLoss() {
  if (!audioUnlocked || !audioCtx) return;
  try {
    const now=audioCtx.currentTime, o=audioCtx.createOscillator(), g=audioCtx.createGain();
    o.connect(g); g.connect(audioCtx.destination);
    o.type='sawtooth'; o.frequency.setValueAtTime(220,now); o.frequency.linearRampToValueAtTime(110,now+0.2);
    g.gain.setValueAtTime(0.2,now); g.gain.exponentialRampToValueAtTime(0.001,now+0.25);
    o.start(now); o.stop(now+0.3);
  } catch(e) {}
}

function flashWin(sym, pnl) {
  const el = document.getElementById('win-flash');
  if (!el) return;
  el.textContent = sym + '  +' + (+pnl).toFixed(2) + 'bp';
  el.classList.add('show');
  setTimeout(() => el.classList.remove('show'), 2200);
}

function loadTrades() {
  try { const r = localStorage.getItem(STORAGE_KEY); localTrades = r ? JSON.parse(r) : []; }
  catch(e) { localTrades = []; }
}
function saveTrades() {
  try { localStorage.setItem(STORAGE_KEY, JSON.stringify(localTrades.slice(0,200))); } catch(e) {}
}
window.clearTrades = function() {
  localTrades=[]; wins=0; losses=0;
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
        const tsStr = tr.t ? (tr.t.length<12 ? new Date().toISOString().slice(0,10)+'T'+tr.t+'Z'
          : (tr.t.endsWith('Z')||tr.t.includes('+')?tr.t:tr.t+'Z')) : null;
        const age = tsStr ? (Date.now()-new Date(tsStr).getTime()) : 99999;
        const fresh = age < 60000;
        if (+tr.p>0) { wins++; if(fresh){playWin();flashWin(tr.s,tr.p);} }
        else if (+tr.p<0) { losses++; if(fresh) playLoss(); }
        else wins++;
      } else { if(+tr.p>0) wins++; else if(+tr.p<0) losses++; }
    }
  });
  if (newCount>0||before===0) { localTrades=localTrades.slice(0,200); saveTrades(); renderTradeLog(); updateWinRate(); }
}

const $ = id => document.getElementById(id);
const set = (id,v) => { const e=$(id); if(e) e.textContent=v; };
const fmtPnl = v => (v>=0?'+':'')+Number(v).toFixed(2)+'bp';
const bpToUsd = bp => (bp/10000)*10000;
const fmtUsd = v => (v>=0?'+$':'-$')+Math.abs(v).toFixed(2);
function fmtHold(ms) {
  if (!ms||ms<=0) return '--';
  if (ms<1000) return ms+'ms';
  if (ms<60000) return (ms/1000).toFixed(1)+'s';
  return Math.floor(ms/60000)+'m'+Math.floor((ms%60000)/1000)+'s';
}
function fmtPrice(p, sym) {
  if (!p||p<=0) return '--';
  const s=(sym||'').toUpperCase();
  if (s==='SOL'||s==='LINK'||s==='POL') return '$'+Number(p).toFixed(3);
  if (s==='AVAX') return '$'+Number(p).toFixed(2);
  return '$'+Number(p).toLocaleString('en-US',{minimumFractionDigits:2,maximumFractionDigits:2});
}
function normalizeReason(r) {
  if (!r) return 'TO';
  const u=r.toUpperCase();
  if (u==='SC') return 'SL';
  if (u.startsWith('TRAIL')) return 'TRAIL';
  if (u==='TIMEOUT') return 'TO';
  return u;
}

function updateWinRate() {
  const t=wins+losses;
  const wrEl=$('ts-wr');
  if (wrEl) { wrEl.textContent=t>0?(wins/t*100).toFixed(0)+'%':'--%'; wrEl.className='sc-val '+(t>0?(wins>=losses?'pos':'neg'):''); }
}

function updateUptime() {
  if (!uptimeStart) return;
  const s=Math.floor((Date.now()-uptimeStart)/1000);
  set('tb-uptime', String(Math.floor(s/3600)).padStart(2,'0')+':'+String(Math.floor((s%3600)/60)).padStart(2,'0')+':'+String(s%60).padStart(2,'0'));
}

// Readiness colour from vol + displacement + compression
function readinessStyle(d) {
  if (!d) return {bg:'var(--dim)',shadow:'none'};
  const vol  = Math.min(1, Math.max(0, ((d.vol_ratio||1)-1.0)/0.8));
  const disp = Math.min(1, Math.abs(d.displacement_bp||0)/30);
  const comp = Math.min(1, (d.compression_ticks||0)/100);
  const score = vol*0.5 + disp*0.3 + comp*0.2;
  if (score < 0.15) return {bg:'var(--dim)',shadow:'none'};
  if (score < 0.4)  return {bg:'rgba(255,214,0,.7)',shadow:'0 0 6px rgba(255,214,0,.4)'};
  if (score < 0.65) return {bg:'rgba(0,212,255,.8)',shadow:'0 0 8px rgba(0,212,255,.4)'};
  return {bg:'rgba(0,230,118,.9)',shadow:'0 0 10px rgba(0,230,118,.5)'};
}

function regimePillClass(state) {
  if (!state) return 'rp-neutral';
  const s=state.toUpperCase();
  if (s.includes('BREAKOUT')||s.includes('BURST')) return 'rp-breakout';
  if (s.includes('BUILDUP')||s.includes('TREND')) return 'rp-buildup';
  if (s.includes('COMPRESSION')) return 'rp-compression';
  if (s.includes('DEAD')) return 'rp-dead';
  return 'rp-neutral';
}

function renderTradeLog() {
  let totalPnl=0,winPnl=0,lossPnl=0,winCount=0,lossCount=0,tp=0,sl=0,trail=0,timeout=0;
  currentSessionTrades().forEach(t => {
    const p=+t.p||0; totalPnl+=p;
    const why=normalizeReason(t.why||t.reason||'');
    if(why==='TP')tp++; else if(why==='SL')sl++; else if(why==='TRAIL')trail++; else timeout++;
    if(p>=0){winPnl+=p;winCount++;}else{lossPnl+=p;lossCount++;}
  });
  const avgWin=winCount>0?winPnl/winCount:null;
  const avgLoss=lossCount>0?lossPnl/lossCount:null;
  const total=winCount+lossCount;
  const wr=total>0?winCount/total:null;
  const exp=(wr!==null&&avgWin!==null&&avgLoss!==null)?(wr*avgWin+(1-wr)*avgLoss):null;

  set('ts-count',total); set('ts-wins',winCount); set('ts-losses',lossCount);
  const pEl=$('ts-pnl'); if(pEl){pEl.textContent=fmtPnl(totalPnl);pEl.className='sc-val '+(totalPnl>=0?'pos':'neg');}
  const wrEl=$('ts-wr'); if(wrEl){wrEl.textContent=wr!==null?(wr*100).toFixed(0)+'%':'--%';wrEl.className='sc-val '+(wr!==null?(wr>=0.5?'pos':'neg'):'');}
  const awEl=$('ts-avgwin'); if(awEl)awEl.textContent=avgWin!==null?'+'+avgWin.toFixed(2)+'bp':'--';
  const alEl=$('ts-avgloss'); if(alEl)alEl.textContent=avgLoss!==null?avgLoss.toFixed(2)+'bp':'--';
  const expEl=$('ts-exp'); if(expEl){expEl.textContent=exp!==null?(exp>=0?'+':'')+exp.toFixed(2)+'bp':'--';expEl.className='sc-val '+(exp!==null?(exp>=0?'pos':'neg'):'');}
  set('st-tp',tp); set('st-sl',sl); set('st-trail',trail); set('st-timeout',timeout);
  set('st-trail-hd',trail);

  set('tp-wins',winCount); set('tp-losses',lossCount);
  const tpWr=$('tp-wr'); if(tpWr){tpWr.textContent=wr!==null?(wr*100).toFixed(0)+'%':'--%';tpWr.className=wr!==null?(wr>=0.5?'pos':'neg'):'';}
  const tpPnl=$('tp-pnl'); if(tpPnl){tpPnl.textContent=fmtPnl(totalPnl);tpPnl.className=totalPnl>=0?'pos':'neg';}

  const body=$('btm-trade-rows');
  if (!body) return;
  const trades=currentSessionTrades();
  if (!trades.length){body.innerHTML='<div class="tl-empty">Waiting for first trade...</div>';return;}
  body.innerHTML=trades.slice(0,60).map(tr => {
    const p=+tr.p||0, isWin=p>=0;
    const sym=(tr.s||'').replace('USDT','').replace('/','');
    const eng=(tr.e||'?').toUpperCase();
    const why=normalizeReason(tr.why||tr.reason||'?');
    const whyCls=why==='TP'?'why-tp':why==='SL'?'why-sl':why==='TRAIL'?'why-trail':'why-to';
    const time=tr.t?(tr.t.length>10?tr.t.substring(11,16):tr.t):'--';
    const mfe=tr.mfe!=null?'+'+Number(tr.mfe).toFixed(1)+'bp':'--';
    const mae=tr.mae!=null?Number(tr.mae).toFixed(1)+'bp':'--';
    const hold=tr.hold!=null?fmtHold(tr.hold):'--';
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

let firstPoll=true, lastKnownUptimeHours=null;

function updateAll(data) {
  if (!data) return;
  if (!uptimeStart) uptimeStart=Date.now();

  const serverUptime=data.uptime_hours||0;
  if (lastKnownUptimeHours!==null && serverUptime<lastKnownUptimeHours-0.01) {
    localTrades=[]; try{localStorage.removeItem(STORAGE_KEY);}catch(e){}
    uptimeStart=Date.now(); firstPoll=true;
  }
  lastKnownUptimeHours=serverUptime;

  // Topbar
  const pnl=data.pnl||0;
  const pnlEl=$('tb-pnl'); if(pnlEl){pnlEl.textContent=fmtPnl(pnl);pnlEl.className='tb-val '+(pnl>0?'pos':pnl<0?'neg':'');}
  const eq=$('tb-equity'); if(eq)eq.textContent='$'+(10000+pnl).toLocaleString('en-US',{minimumFractionDigits:2,maximumFractionDigits:2});
  set('tb-trades',data.total_trades||0);
  set('tb-positions',data.open_positions||0);
  if (data.build_ver){const bv=$('build-ver');if(bv)bv.textContent='v'+data.build_ver;}
  const lat=data.latency_p95||0;
  const latEl=$('tb-latency');
  if (latEl){latEl.textContent=lat>0?lat.toFixed(1)+'ms':'--ms';latEl.className='tb-val accent '+(lat<=0?'':lat<25?'pos':lat<50?'':'neg');}

  // Symbol cards
  SYMBOLS.forEach(({ short, full }) => {
    const sym=short.toLowerCase();
    const d=data[full];
    const px=data[full+'_price']||0;

    // Price
    const pxEl=$('px-'+sym);
    if (pxEl){
      pxEl.textContent=fmtPrice(px,short);
      pxEl.className='sym-price'+(px>lastPrices[sym]?' up':px<lastPrices[sym]?' down':'');
    }
    lastPrices[sym]=px;

    if (!d) return;

    // Readiness bar
    const bar=$('bar-'+sym);
    if (bar){const rs=readinessStyle(d);bar.style.background=rs.bg;bar.style.boxShadow=rs.shadow;}

    // Regime
    const state=d.regime_state||'NEUTRAL';
    const reg=$('reg-'+sym);
    if (reg){reg.textContent=state;reg.className='sym-regime-pill '+regimePillClass(state);}

    // Metrics
    set('vr-'+sym,  d.vol_ratio!=null?d.vol_ratio.toFixed(2):'--');
    set('dp-'+sym,  d.displacement_bp!=null?d.displacement_bp.toFixed(1)+'bp':'--');
    set('cap-'+sym, d.dynamic_cap_R!=null?d.dynamic_cap_R.toFixed(1)+'R':'--');
    const rmEl=$('rm-'+sym);
    if (rmEl){
      const mult=d.regime_multiplier||1;
      rmEl.textContent=mult.toFixed(2)+'x';
      rmEl.className='sm-val '+(mult>1.1?'pos':mult<0.9?'neg':'');
    }

    // Card active glow
    const card=$('sb-'+sym);
    if (card) card.className='sym-card'+(d.micro_active?' active':'');

    // Engine pips
    const engs=[['micro',d.micro_active],['structural',d.structural_active],['convex',d.convex_active],['compression',d.compression_active]];
    let anyActive=false;
    engs.forEach(([eng,active]) => {
      const pip=$('ed-'+sym+'-'+eng);
      if (pip) pip.className='eng-pip'+(active?' active':'');
      if (active) anyActive=true;
    });
    const lbl=$('edl-'+sym);
    if (lbl){lbl.textContent=anyActive?'ACTIVE':'FLAT';lbl.className='eng-label'+(anyActive?' active':'');}

    // P&L + trades
    const symTrades=currentSessionTrades().filter(t=>(t.s||'').replace('USDT','').toUpperCase()===short);
    const symPnl=symTrades.reduce((a,t)=>a+(+t.p||0),0);
    const miniPnl=$('mini-pnl-'+sym);
    if (miniPnl){miniPnl.textContent=(symPnl>=0?'+':'')+symPnl.toFixed(2)+'bp';miniPnl.className='sym-pnl '+(symPnl>0?'pos':symPnl<0?'neg':'zero');}
    set('mini-t-'+sym,(symTrades.length||0)+'T');
  });

  // By-layer breakdown
  if (data.session && data.session.by_layer) {
    const s=data.session;
    const bl=$('st-by-layer');
    if (bl && s.by_layer && s.by_layer.length) {
      bl.innerHTML=s.by_layer.map(l =>
        '<div class="layer-row">'
        +'<span class="lr-name">'+l.name+'</span>'
        +'<div class="lr-stats">'
        +'<span>'+l.trades+'T</span>'
        +'<span class="lr-wr '+(l.wr>=50?'good':'bad')+'">'+Math.round(l.wr)+'%WR</span>'
        +'<span class="lr-pnl '+(l.pnl>=0?'pos':'neg')+'">'+(l.pnl>=0?'+':'')+Number(l.pnl).toFixed(2)+'bp</span>'
        +'</div></div>'
      ).join('');
    }
    set('st-tp',s.tp_exits||0); set('st-sl',s.sl_exits||0);
    set('st-trail',s.trail_exits||0); set('st-trail-hd',s.trail_exits||0); set('st-timeout',s.timeout_exits||0);
  }

  if (data.trade_log) mergeTrades(data.trade_log, firstPoll);
  firstPoll=false;
}

// POLL
let connected=false, pollErrors=0, lastPollOk=null;
function fmtAgo(ts){if(!ts)return'never';const s=Math.floor((Date.now()-ts)/1000);return s<60?s+'s ago':Math.floor(s/60)+'m'+(s%60)+'s ago';}

function setPollOk(){
  pollErrors=0;lastPollOk=Date.now();connected=true;
  const cb=$('conn-badge');if(cb){cb.textContent='LIVE';cb.className='badge badge-conn';}
  const d=$('live-dot');if(d)d.className='dot live';
  const b=$('poll-error');if(b)b.classList.remove('show');
}
function setPollError(reason){
  pollErrors++;connected=false;
  const cb=$('conn-badge');if(cb){cb.textContent='OFFLINE';cb.className='badge badge-disc';}
  const d=$('live-dot');if(d)d.className='dot';
  const b=$('poll-error');if(b)b.classList.add('show');
  const m=$('poll-error-msg');if(m)m.textContent=reason;
  const c=$('poll-error-count');if(c)c.textContent='ERR'+pollErrors;
  const e=$('poll-error-time');if(e)e.textContent='last ok: '+fmtAgo(lastPollOk);
}

async function poll(){
  let res;
  try{res=await fetch('/api/state',{cache:'no-store',signal:AbortSignal.timeout(4000)});}
  catch(e){setPollError(e.name==='TimeoutError'?'Fetch timeout':'Network error');return;}
  if(!res.ok){setPollError('HTTP '+res.status);return;}
  let data;
  try{data=await res.json();}catch(e){setPollError('JSON parse error');return;}
  setPollOk(); updateAll(data);
}

setInterval(()=>{if(pollErrors>0){const e=$('poll-error-time');if(e)e.textContent='last ok: '+fmtAgo(lastPollOk);}},1000);

loadTrades(); renderTradeLog(); updateWinRate();
wins=0;losses=0;
currentSessionTrades().forEach(t=>{if(+t.p>0)wins++;else if(+t.p<0)losses++;});
updateWinRate();
poll();
setInterval(poll,1000);
setInterval(updateUptime,1000);
