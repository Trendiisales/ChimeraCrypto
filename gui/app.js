(function() {
    'use strict';

    const $ = (id) => document.getElementById(id);
    const fmt = (v, d = 2) => (v == null || v === '--') ? '--' : Number(v).toFixed(d);

    // ── WIN BELL ─────────────────────────────────────────────────────────────
    // Synthesised using Web Audio API - no external files needed.
    // Plays a warm two-tone chime on every winning trade.
    let audioCtx = null;

    function playWinBell() {
        try {
            if (!audioCtx) audioCtx = new (window.AudioContext || window.webkitAudioContext)();
            const ctx = audioCtx;

            function tone(freq, startTime, duration, gainPeak) {
                const osc  = ctx.createOscillator();
                const gain = ctx.createGain();
                osc.connect(gain);
                gain.connect(ctx.destination);
                osc.type = 'sine';
                osc.frequency.setValueAtTime(freq, startTime);
                gain.gain.setValueAtTime(0, startTime);
                gain.gain.linearRampToValueAtTime(gainPeak, startTime + 0.01);
                gain.gain.exponentialRampToValueAtTime(0.001, startTime + duration);
                osc.start(startTime);
                osc.stop(startTime + duration);
            }

            const t = ctx.currentTime;
            tone(880, t,        0.6, 0.4);   // A5 - bright attack
            tone(1108, t + 0.05, 0.5, 0.25); // C#6 - harmonic
            tone(659,  t + 0.15, 0.8, 0.2);  // E5  - warm sustain
        } catch(e) {
            console.warn('Bell failed:', e);
        }
    }

    // Flash a green WIN banner at top of screen
    function flashWin(symbol, pnlBp) {
        let banner = $('win-banner');
        if (!banner) {
            banner = document.createElement('div');
            banner.id = 'win-banner';
            banner.style.cssText = [
                'position:fixed', 'top:0', 'left:0', 'right:0',
                'background:linear-gradient(90deg,#00c853,#69f0ae)',
                'color:#000', 'font-weight:700', 'font-size:18px',
                'text-align:center', 'padding:10px',
                'z-index:9999', 'display:none',
                'letter-spacing:2px', 'box-shadow:0 2px 20px #00c85388'
            ].join(';');
            document.body.prepend(banner);
        }
        banner.textContent = `✅ WIN  ${symbol.toUpperCase()}  +${Number(pnlBp).toFixed(2)}bp`;
        banner.style.display = 'block';
        setTimeout(() => { banner.style.display = 'none'; }, 3000);
    }

    // ── CLOCK ─────────────────────────────────────────────────────────────────
    setInterval(() => {
        const el = $('system-time');
        if (el) el.textContent = new Date().toLocaleTimeString('en-US', {hour12: false});
    }, 1000);

    // ── PRICE UPDATE ──────────────────────────────────────────────────────────
    function updatePrices(data) {
        if (data.btc_price > 0) {
            const el = $('btc-live-price');
            if (el) el.textContent = '$' + fmt(data.btc_price, 2);
        }
        if (data.eth_price > 0) {
            const el = $('eth-live-price');
            if (el) el.textContent = '$' + fmt(data.eth_price, 2);
        }
        if (data.sol_price > 0) {
            const el = $('sol-live-price');
            if (el) el.textContent = '$' + fmt(data.sol_price, 2);
        }
    }

    // ── ENGINE STATE UPDATE ───────────────────────────────────────────────────
    function updateSymbolEngines(symbol, data) {
        const prefix = symbol.substring(0, 3);
        const el = (id) => $(prefix + '-' + id);

        if (el('portfolio-r')) el('portfolio-r').textContent = `Portfolio: ${fmt(data.portfolio_R, 2)}R`;

        // MICRO
        const microStatus = el('micro-status');
        if (microStatus) {
            microStatus.textContent = data.micro_active ? 'ACTIVE' : 'INACTIVE';
            microStatus.className = 'engine-status ' + (data.micro_active ? 'active' : 'inactive');
        }
        if (el('micro-size'))  el('micro-size').textContent  = data.micro_active ? '1.0R' : '0.00R';
        if (el('micro-entry')) el('micro-entry').textContent = data.micro_entry_price > 0 ? fmt(data.micro_entry_price, 2) : '--';
        if (el('micro-mfe')) {
            el('micro-mfe').textContent = fmt(data.micro_mfe_bp, 2) + 'bp';
            el('micro-mfe').className = 'engine-stat-value ' + (data.micro_mfe_bp > 0 ? 'positive' : '');
        }
        if (el('micro-pnl')) {
            el('micro-pnl').textContent = fmt(data.micro_total_pnl_bp, 2) + 'bp';
            el('micro-pnl').className = 'engine-stat-value ' + (data.micro_total_pnl_bp > 0 ? 'positive' : data.micro_total_pnl_bp < 0 ? 'negative' : '');
        }

        // STRUCTURAL
        const structStatus = el('structural-status');
        if (structStatus) {
            structStatus.textContent = data.structural_active ? 'ACTIVE' : 'INACTIVE';
            structStatus.className = 'engine-status ' + (data.structural_active ? 'active' : 'inactive');
        }
        if (el('structural-size'))  el('structural-size').textContent  = fmt(data.structural_size_R, 2) + 'R';
        if (el('structural-entry')) el('structural-entry').textContent = data.structural_entry_price > 0 ? fmt(data.structural_entry_price, 2) : '--';
        if (el('structural-mfe')) {
            el('structural-mfe').textContent = fmt(data.structural_mfe_bp, 2) + 'bp';
            el('structural-mfe').className = 'engine-stat-value ' + (data.structural_mfe_bp > 0 ? 'positive' : '');
        }
        if (el('structural-pnl')) {
            el('structural-pnl').textContent = fmt(data.structural_total_pnl_bp, 2) + 'bp';
            el('structural-pnl').className = 'engine-stat-value ' + (data.structural_total_pnl_bp > 0 ? 'positive' : data.structural_total_pnl_bp < 0 ? 'negative' : '');
        }

        // CONVEX
        const convexStatus = el('convex-status');
        if (convexStatus) {
            convexStatus.textContent = data.convex_active ? 'ACTIVE' : 'INACTIVE';
            convexStatus.className = 'engine-status ' + (data.convex_active ? 'active' : 'inactive');
        }
        if (el('convex-size'))  el('convex-size').textContent  = fmt(data.convex_size_R, 2) + 'R';
        if (el('convex-entry')) el('convex-entry').textContent = data.convex_entry_price > 0 ? fmt(data.convex_entry_price, 2) : '--';
        if (el('convex-mfe')) {
            el('convex-mfe').textContent = fmt(data.convex_mfe_bp, 2) + 'bp';
            el('convex-mfe').className = 'engine-stat-value ' + (data.convex_mfe_bp > 0 ? 'positive' : '');
        }
        if (el('convex-pnl')) {
            el('convex-pnl').textContent = fmt(data.convex_total_pnl_bp, 2) + 'bp';
            el('convex-pnl').className = 'engine-stat-value ' + (data.convex_total_pnl_bp > 0 ? 'positive' : data.convex_total_pnl_bp < 0 ? 'negative' : '');
        }

        // COMPRESSION
        const compressionStatus = el('compression-status');
        if (compressionStatus) {
            if (data.compression_active) {
                compressionStatus.textContent = 'ACTIVE';
                compressionStatus.className = 'engine-status active';
            } else if (data.compression_ticks && data.compression_ticks > 50) {
                compressionStatus.textContent = 'ARMED';
                compressionStatus.className = 'engine-status active';
            } else {
                compressionStatus.textContent = 'INACTIVE';
                compressionStatus.className = 'engine-status inactive';
            }
        }
        if (el('compression-size'))  el('compression-size').textContent  = fmt(data.compression_size_R, 2) + 'R';
        if (el('compression-entry')) el('compression-entry').textContent = data.compression_entry_price > 0 ? fmt(data.compression_entry_price, 2) : '--';
        if (el('compression-mfe')) {
            el('compression-mfe').textContent = fmt(data.compression_mfe_bp, 2) + 'bp';
            el('compression-mfe').className = 'engine-stat-value ' + (data.compression_mfe_bp > 0 ? 'positive' : '');
        }
        if (el('compression-pnl')) {
            el('compression-pnl').textContent = fmt(data.compression_total_pnl_bp, 2) + 'bp';
            el('compression-pnl').className = 'engine-stat-value ' + (data.compression_total_pnl_bp > 0 ? 'positive' : data.compression_total_pnl_bp < 0 ? 'negative' : '');
        }
    }

    function updateTelemetry(data) {
        if (!data) return;
        try {
            updatePrices(data);
            if (data.btcusdt) updateSymbolEngines('btcusdt', data.btcusdt);
            if (data.ethusdt) updateSymbolEngines('ethusdt', data.ethusdt);
            if (data.solusdt) updateSymbolEngines('solusdt', data.solusdt);
            const health = $('health-indicator');
            if (health) health.className = 'health-dot active';
        } catch(e) {
            console.error('Update error:', e);
        }
    }

    // ── WEBSOCKET - real-time trade events ───────────────────────────────────
    let ws = null;
    let wsReconnectDelay = 1000;

    function connectWS() {
        const host = window.location.hostname || '154.45.251.118';
        const url  = `ws://${host}:9001`;

        try {
            ws = new WebSocket(url);
        } catch(e) {
            scheduleReconnect();
            return;
        }

        ws.onopen = () => {
            console.log('[WS] Connected to chimera engine');
            wsReconnectDelay = 1000;
            const el = $('connection-status');
            if (el) { el.textContent = 'Live'; el.className = 'footer-value connected'; }
        };

        ws.onmessage = (event) => {
            try {
                const msg = JSON.parse(event.data);
                handleEngineMessage(msg);
            } catch(e) {}
        };

        ws.onclose = () => {
            const el = $('connection-status');
            if (el) { el.textContent = 'Reconnecting...'; el.className = 'footer-value disconnected'; }
            scheduleReconnect();
        };

        ws.onerror = () => { ws.close(); };
    }

    function scheduleReconnect() {
        setTimeout(connectWS, wsReconnectDelay);
        wsReconnectDelay = Math.min(wsReconnectDelay * 2, 10000);
    }

    // ── ENGINE MESSAGE HANDLER ────────────────────────────────────────────────
    function handleEngineMessage(msg) {
        if (!msg || !msg.type) return;

        switch (msg.type) {

            // Trade exit - check for win, fire bell
            case 'position_exit':
            case 'trade': {
                const pnl = parseFloat(msg.pnl_bps || msg.last_order_conviction || 0);
                const sym = msg.symbol || msg.last_order_symbol || '';
                if (pnl > 0) {
                    playWinBell();
                    flashWin(sym, pnl);
                }
                updateTradeLog(msg, pnl);
                break;
            }

            // Snapshot update
            case 'snapshot':
            case 'desk_snapshot': {
                updateTelemetry(msg);
                updateEquityBar(msg);
                break;
            }

            // Regime change
            case 'regime_update': {
                updateRegimeIndicator(msg);
                break;
            }

            // Performance
            case 'performance_summary': {
                updatePerformance(msg);
                break;
            }
        }
    }

    // ── TRADE LOG ─────────────────────────────────────────────────────────────
    const tradeLog = [];

    function updateTradeLog(msg, pnl) {
        const entry = {
            time:   new Date().toLocaleTimeString('en-US', {hour12: false}),
            symbol: (msg.symbol || msg.last_order_symbol || '???').toUpperCase().replace('USDT',''),
            pnl:    pnl,
            layer:  msg.layer || '?'
        };
        tradeLog.unshift(entry);
        if (tradeLog.length > 50) tradeLog.pop();

        const container = $('trade-log');
        if (!container) return;

        const wins   = tradeLog.filter(t => t.pnl > 0).length;
        const losses = tradeLog.filter(t => t.pnl < 0).length;
        const total  = wins + losses;
        const wr     = total > 0 ? (wins / total * 100).toFixed(0) : '--';

        container.innerHTML =
            `<div style="font-size:11px;color:#888;margin-bottom:6px">` +
            `Recent Trades | W:${wins} L:${losses} | WR:${wr}%</div>` +
            tradeLog.slice(0, 20).map(t => {
                const color = t.pnl > 0 ? '#00c853' : '#ff5252';
                const sign  = t.pnl > 0 ? '+' : '';
                return `<div style="font-size:12px;padding:2px 0;border-bottom:1px solid #1a1a2e">` +
                       `<span style="color:#888">${t.time}</span> ` +
                       `<span style="color:#00d4ff">${t.symbol}</span> ` +
                       `<span style="color:${color};font-weight:600">${sign}${t.pnl.toFixed(2)}bp</span> ` +
                       `<span style="color:#555">${t.layer}</span></div>`;
            }).join('');
    }

    function updateEquityBar(msg) {
        const equity = parseFloat(msg.equity || 0);
        const pnl    = parseFloat(msg.day_pnl || msg.pnl || 0);
        if (!equity) return;

        const el = $('equity-value');
        if (el) el.textContent = '$' + fmt(equity, 2);

        const pnlEl = $('day-pnl');
        if (pnlEl) {
            const sign = pnl >= 0 ? '+' : '';
            pnlEl.textContent   = sign + fmt(pnl, 2) + 'bp';
            pnlEl.style.color   = pnl > 0 ? '#00c853' : pnl < 0 ? '#ff5252' : '#888';
        }
    }

    function updateRegimeIndicator(msg) {
        if (!msg.symbol) return;
        const prefix = msg.symbol.substring(0, 3).toUpperCase();
        const el = $('regime-' + prefix);
        if (el) {
            el.textContent = msg.regime_name || '?';
            el.style.color = msg.regime_name === 'BURST' ? '#00c853' :
                             msg.regime_name === 'REVERT' ? '#ff5252' : '#888';
        }
    }

    function updatePerformance(msg) {
        const el = $('total-pnl');
        if (el && msg.total_pnl != null) {
            const sign = msg.total_pnl >= 0 ? '+' : '';
            el.textContent = sign + fmt(msg.total_pnl, 2) + 'bp';
            el.style.color = msg.total_pnl > 0 ? '#00c853' : '#ff5252';
        }
    }

    // ── HTTP POLL - fallback state ────────────────────────────────────────────
    function pollState() {
        fetch('/api/state')
            .then(r => r.json())
            .then(data => {
                updateTelemetry(data);
                // Only update connection status if WS not already connected
                if (!ws || ws.readyState !== WebSocket.OPEN) {
                    const el = $('connection-status');
                    if (el) { el.textContent = 'HTTP'; el.className = 'footer-value connected'; }
                }
            })
            .catch(() => {
                if (!ws || ws.readyState !== WebSocket.OPEN) {
                    const el = $('connection-status');
                    if (el) { el.textContent = 'Offline'; el.className = 'footer-value disconnected'; }
                }
            });
    }

    setInterval(pollState, 1000);
    pollState();

    // Start WebSocket for real-time bells and trade events
    connectWS();

})();
