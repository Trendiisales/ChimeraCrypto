(function() {
    'use strict';
    
    const tradeHistory = [];
    const maxTrades = 20;
    
    const $ = (id) => document.getElementById(id);
    const fmt = (v, d = 2) => (v == null || v === '--') ? '--' : Number(v).toFixed(d);
    const fmtUSD = (v) => {
        if (v == null || v === '--') return '--';
        const n = Number(v);
        return '$' + (Math.abs(n) >= 1000 ? (n/1000).toFixed(2) + 'k' : n.toFixed(2));
    };
    const fmtPct = (v) => (v == null || v === '--') ? '--' : (Number(v) * 100).toFixed(2) + '%';
    
    // Update system clock every second
    setInterval(() => {
        const now = new Date();
        $('system-time').textContent = now.toLocaleTimeString('en-US', {hour12: false});
    }, 1000);
    
    function updatePnL(id, value) {
        const el = $(id);
        if (!el) return;
        const num = Number(value);
        el.textContent = fmtUSD(value);
        el.classList.remove('positive', 'negative');
        if (num > 0.01) el.classList.add('positive');
        else if (num < -0.01) el.classList.add('negative');
    }
    
    function updatePrice(symbol, price, change) {
        const priceEl = $(symbol + '_price');
        const changeEl = $(symbol + '_change');
        if (priceEl && price > 0) {
            priceEl.textContent = '$' + fmt(price, 2);
        }
        if (changeEl) {
            const pct = Number(change);
            changeEl.textContent = (pct > 0 ? '+' : '') + fmt(pct, 3) + '%';
            changeEl.classList.remove('positive', 'negative');
            if (pct > 0) changeEl.classList.add('positive');
            else if (pct < 0) changeEl.classList.add('negative');
        }
    }
    
    function addTrade(data) {
        if (!data.last_order_symbol || !data.last_order_side) return;
        
        const trade = {
            time: data.last_order_time || new Date().toLocaleTimeString(),
            symbol: data.last_order_symbol.replace('usdt', '').toUpperCase(),
            side: data.last_order_side,
            size: data.last_order_size,
            price: data.last_order_price,
            conviction: data.last_order_conviction || 0,
            cost_floor: data.last_order_cost_floor || 0,
            pnl: data.day_pnl || 0
        };
        
        if (tradeHistory.length === 0 || 
            tradeHistory[0].time !== trade.time ||
            tradeHistory[0].symbol !== trade.symbol) {
            
            tradeHistory.unshift(trade);
            if (tradeHistory.length > maxTrades) {
                tradeHistory.pop();
            }
            
            renderTrades();
        }
    }
    
    function renderTrades() {
        const feed = $('trade-feed');
        if (!feed) return;
        
        if (tradeHistory.length === 0) {
            feed.innerHTML = '<div class="no-trades">Waiting for trades...</div>';
            return;
        }
        
        let html = '';
        for (let i = 0; i < tradeHistory.length; i++) {
            const t = tradeHistory[i];
            const pnlChange = i < tradeHistory.length - 1 ? 
                (t.pnl - tradeHistory[i + 1].pnl) : t.pnl;
            
            const pnlClass = pnlChange > 0 ? 'positive' : pnlChange < 0 ? 'negative' : '';
            const sideClass = t.side.toLowerCase();
            const edgePass = t.conviction > t.cost_floor;
            
            html += `
                <div class="trade-item ${sideClass}">
                    <div class="trade-time">${t.time}</div>
                    <div class="trade-symbol">${t.symbol}</div>
                    <div class="trade-side ${sideClass}">${t.side}</div>
                    <div class="trade-size">${fmt(t.size, 4)}</div>
                    <div class="trade-price">$${fmt(t.price, 2)}</div>
                    <div class="trade-conviction ${edgePass ? 'positive' : 'negative'}">${fmt(t.conviction, 1)} bps</div>
                    <div class="trade-pnl ${pnlClass}">${pnlChange > 0 ? '+' : ''}$${fmt(pnlChange, 2)}</div>
                </div>
            `;
        }
        
        feed.innerHTML = html;
        $('trade-count').textContent = tradeHistory.length;
    }
    
    function updateTelemetry(d) {
        try {
            if (d.btc_price) updatePrice('btc', d.btc_price, d.btc_change_pct);
            if (d.eth_price) updatePrice('eth', d.eth_price, d.eth_change_pct);
            if (d.sol_price) updatePrice('sol', d.sol_price, d.sol_change_pct);
            
            if (d.equity) $('equity').textContent = fmtUSD(d.equity);
            if (d.day_pnl !== undefined) updatePnL('day_pnl', d.day_pnl);
            if (d.pnl !== undefined) updatePnL('pnl', d.pnl);
            if (d.unrealized_pnl !== undefined) updatePnL('unrealized_pnl', d.unrealized_pnl);
            
            // FIX LATENCY DISPLAY
            if (d.latency_ms !== undefined && d.latency_ms > 0) {
                $('latency_ms').textContent = fmt(d.latency_ms, 1) + 'ms';
            }
            
            if (d.orders_sent !== undefined) $('orders_sent').textContent = d.orders_sent;
            if (d.fills_received !== undefined) $('fills_received').textContent = d.fills_received;
            
            if (d.governor) $('governor').textContent = d.governor;
            if (d.exposure_usd) $('exposure_usd').textContent = fmtUSD(d.exposure_usd);
            if (d.positions !== undefined) $('positions').textContent = d.positions;
            
            if (d.win_rate !== undefined) $('win_rate').textContent = fmtPct(d.win_rate);
            if (d.sharpe_ratio !== undefined) $('sharpe_ratio').textContent = fmt(d.sharpe_ratio, 2);
            if (d.trades_today !== undefined) $('trades_today').textContent = d.trades_today;
            
            if (d.btc_position !== undefined) $('btc_position').textContent = fmt(d.btc_position, 4);
            if (d.eth_position !== undefined) $('eth_position').textContent = fmt(d.eth_position, 4);
            if (d.sol_position !== undefined) $('sol_position').textContent = fmt(d.sol_position, 4);
            
            if (d.uptime_hours) $('uptime').textContent = fmt(d.uptime_hours, 1) + 'h';
            if (d.mode) $('mode').textContent = d.mode;
            if (d.healthy !== undefined) {
                const el = $('health-indicator');
                if (el) {
                    el.style.background = d.healthy ? '#10b981' : '#ef4444';
                    el.style.boxShadow = d.healthy ? '0 0 12px #10b981' : '0 0 12px #ef4444';
                }
            }
            
            addTrade(d);
        } catch (err) {
            console.error('Update error:', err);
        }
    }
    
    let ws = null, reconnectTimer = null, attempts = 0;
    function connect() {
        if (reconnectTimer) clearTimeout(reconnectTimer);
        const wsUrl = `${window.location.protocol === 'https:' ? 'wss:' : 'ws:'}//${window.location.host}/ws`;
        const el = $('ws-status');
        if (el) { el.textContent = 'Connecting...'; el.className = 'footer-value connecting'; }
        
        ws = new WebSocket(wsUrl);
        ws.onopen = () => { 
            attempts = 0; 
            if (el) { el.textContent = 'Connected'; el.className = 'footer-value connected'; } 
        };
        ws.onmessage = (e) => { 
            try { updateTelemetry(JSON.parse(e.data)); } catch(err){} 
        };
        ws.onerror = () => { if (el) { el.textContent = 'Error'; el.className = 'footer-value disconnected'; } };
        ws.onclose = () => {
            if (el) { el.textContent = 'Disconnected'; el.className = 'footer-value disconnected'; }
            attempts++;
            reconnectTimer = setTimeout(connect, Math.min(1000 * Math.pow(2, attempts), 10000));
        };
    }
    
    document.addEventListener('DOMContentLoaded', connect);
    window.addEventListener('beforeunload', () => { 
        if (ws) ws.close(); 
        if (reconnectTimer) clearTimeout(reconnectTimer); 
    });
})();
