(function() {
    'use strict';
    
    const $ = (id) => document.getElementById(id);
    const fmt = (v, d = 2) => (v == null || v === '--') ? '--' : Number(v).toFixed(d);
    
    setInterval(() => {
        const now = new Date();
        const el = $('system-time');
        if (el) el.textContent = now.toLocaleTimeString('en-US', {hour12: false});
    }, 1000);
    
    function updatePrices(data) {
        if (data.btc_price && data.btc_price > 0) {
            const el = $('btc-live-price');
            if (el) el.textContent = '$' + fmt(data.btc_price, 2);
        }
        
        if (data.eth_price && data.eth_price > 0) {
            const el = $('eth-live-price');
            if (el) el.textContent = '$' + fmt(data.eth_price, 2);
        }
        
        if (data.sol_price && data.sol_price > 0) {
            const el = $('sol-live-price');
            if (el) el.textContent = '$' + fmt(data.sol_price, 2);
        }
    }
    
    function updateSymbolEngines(symbol, data) {
        const prefix = symbol.substring(0, 3);
        
        const portfolioEl = $(prefix + '-portfolio-r');
        if (portfolioEl) {
            portfolioEl.textContent = `Portfolio: ${fmt(data.portfolio_R, 2)}R`;
        }
        
        const el = (id) => $(prefix + '-' + id);
        
        // === MICRO ENGINE ===
        const microStatus = el('micro-status');
        if (microStatus) {
            if (data.micro_active) {
                microStatus.textContent = 'ACTIVE';
                microStatus.className = 'engine-status active';
            } else {
                microStatus.textContent = 'INACTIVE';
                microStatus.className = 'engine-status inactive';
            }
        }
        
        if (el('micro-size')) el('micro-size').textContent = data.micro_active ? '1.0R' : '0.00R';
        if (el('micro-entry')) el('micro-entry').textContent = data.micro_entry_price > 0 ? 
            fmt(data.micro_entry_price, 2) : '--';
        
        const microMfe = el('micro-mfe');
        if (microMfe) {
            microMfe.textContent = fmt(data.micro_mfe_bp, 2) + 'bp';
            microMfe.className = 'engine-stat-value ' + (data.micro_mfe_bp > 0 ? 'positive' : '');
        }
        
        const microPnl = el('micro-pnl');
        if (microPnl) {
            microPnl.textContent = fmt(data.micro_total_pnl_bp, 2) + 'bp';
            microPnl.className = 'engine-stat-value ' + 
                (data.micro_total_pnl_bp > 0 ? 'positive' : data.micro_total_pnl_bp < 0 ? 'negative' : '');
        }
        
        // === STRUCTURAL ENGINE ===
        const structStatus = el('structural-status');
        if (structStatus) {
            if (data.structural_active) {
                structStatus.textContent = 'ACTIVE';
                structStatus.className = 'engine-status active';
            } else {
                structStatus.textContent = 'INACTIVE';
                structStatus.className = 'engine-status inactive';
            }
        }
        
        if (el('structural-size')) el('structural-size').textContent = fmt(data.structural_size_R, 2) + 'R';
        if (el('structural-entry')) el('structural-entry').textContent = data.structural_entry_price > 0 ? 
            fmt(data.structural_entry_price, 2) : '--';
        
        const structMfe = el('structural-mfe');
        if (structMfe) {
            structMfe.textContent = fmt(data.structural_mfe_bp, 2) + 'bp';
            structMfe.className = 'engine-stat-value ' + (data.structural_mfe_bp > 0 ? 'positive' : '');
        }
        
        const structPnl = el('structural-pnl');
        if (structPnl) {
            structPnl.textContent = fmt(data.structural_total_pnl_bp, 2) + 'bp';
            structPnl.className = 'engine-stat-value ' + 
                (data.structural_total_pnl_bp > 0 ? 'positive' : data.structural_total_pnl_bp < 0 ? 'negative' : '');
        }
        
        // === CONVEX ENGINE ===
        const convexStatus = el('convex-status');
        if (convexStatus) {
            if (data.convex_active) {
                convexStatus.textContent = 'ACTIVE';
                convexStatus.className = 'engine-status active';
            } else {
                convexStatus.textContent = 'INACTIVE';
                convexStatus.className = 'engine-status inactive';
            }
        }
        
        if (el('convex-size')) el('convex-size').textContent = fmt(data.convex_size_R, 2) + 'R';
        if (el('convex-entry')) el('convex-entry').textContent = data.convex_entry_price > 0 ? 
            fmt(data.convex_entry_price, 2) : '--';
        
        const convexMfe = el('convex-mfe');
        if (convexMfe) {
            convexMfe.textContent = fmt(data.convex_mfe_bp, 2) + 'bp';
            convexMfe.className = 'engine-stat-value ' + (data.convex_mfe_bp > 0 ? 'positive' : '');
        }
        
        const convexPnl = el('convex-pnl');
        if (convexPnl) {
            convexPnl.textContent = fmt(data.convex_total_pnl_bp, 2) + 'bp';
            convexPnl.className = 'engine-stat-value ' + 
                (data.convex_total_pnl_bp > 0 ? 'positive' : data.convex_total_pnl_bp < 0 ? 'negative' : '');
        }
        
        // === COMPRESSION ENGINE ===
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
        
        if (el('compression-size')) el('compression-size').textContent = fmt(data.compression_size_R, 2) + 'R';
        if (el('compression-entry')) el('compression-entry').textContent = data.compression_entry_price > 0 ? 
            fmt(data.compression_entry_price, 2) : '--';
        
        const compressionMfe = el('compression-mfe');
        if (compressionMfe) {
            compressionMfe.textContent = fmt(data.compression_mfe_bp, 2) + 'bp';
            compressionMfe.className = 'engine-stat-value ' + (data.compression_mfe_bp > 0 ? 'positive' : '');
        }
        
        const compressionPnl = el('compression-pnl');
        if (compressionPnl) {
            compressionPnl.textContent = fmt(data.compression_total_pnl_bp, 2) + 'bp';
            compressionPnl.className = 'engine-stat-value ' + 
                (data.compression_total_pnl_bp > 0 ? 'positive' : data.compression_total_pnl_bp < 0 ? 'negative' : '');
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
    
    function pollState() {
        fetch('/api/state')
            .then(r => r.json())
            .then(data => {
                updateTelemetry(data);
                const el = $('connection-status');
                if (el) {
                    el.textContent = 'Connected';
                    el.className = 'footer-value connected';
                }
            })
            .catch(e => {
                console.error('Poll error:', e);
                const el = $('connection-status');
                if (el) {
                    el.textContent = 'Error';
                    el.className = 'footer-value disconnected';
                }
            });
    }
    
    setInterval(pollState, 1000);
    pollState();
    
})();
