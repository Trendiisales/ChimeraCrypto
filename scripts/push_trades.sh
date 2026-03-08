#!/bin/bash
# Auto-push trade log to GitHub every 5 minutes
# Start with: nohup ~/ChimeraCrypto/scripts/push_trades.sh &

REPO_DIR="$HOME/ChimeraCrypto"
LOGFILE="$REPO_DIR/logs/push_trades.log"

log() { echo "[$(date '+%Y-%m-%d %H:%M:%S')] $1" >> "$LOGFILE"; }

mkdir -p "$REPO_DIR/logs"
cd "$REPO_DIR" || exit 1
log "push_trades.sh started"

while true; do
    if [ -f "data/trade_log.json" ]; then
        git add data/trade_log.json 2>/dev/null
        if ! git diff --cached --quiet; then
            COUNT=$(wc -l < data/trade_log.json)
            git commit -m "data: trade log — ${COUNT} trades [auto $(date '+%H:%M')]" 2>/dev/null
            if git push origin main 2>/dev/null; then
                log "Pushed OK — ${COUNT} trades"
            else
                log "Push failed — retrying next cycle"
                git reset HEAD~1 2>/dev/null  # unstage failed commit
            fi
        fi
    fi
    sleep 300
done
