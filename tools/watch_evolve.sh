#!/usr/bin/env bash
# Health watch for a running evolve.py search. Prints one line per event:
#   GEN      a generation finished (from the search's own output)
#   PASS     a candidate passed everything / verification lines
#   ERROR    Python traceback or error in the search output
#   STALL    no new evaluation logged for STALL_MIN minutes
#   DEAD     no cosmos_x1.exe running and no new log lines (search may have stopped)
#   TIMEOUT  runs that hit the time limit (logged with empty results)
# Usage: watch_evolve.sh <search output file> <log jsonl> [interval seconds]
OUT="$1"
LOG="$2"
INTERVAL="${3:-120}"
STALL_MIN=90  # a generation is logged only when it finishes (can take ~40-60 min)
last_lines=$(wc -l < "$LOG" 2>/dev/null || echo 0)
last_change=$(date +%s)
seen_out=$(wc -l < "$OUT" 2>/dev/null || echo 0)  # report only lines written after the watch starts
while true; do
  # New lines in the search output.
  if [ -f "$OUT" ]; then
    total=$(wc -l < "$OUT")
    if [ "$total" -gt "$seen_out" ]; then
      tail -n +"$((seen_out + 1))" "$OUT" | grep -E "done in|VERIFIED|verify seed|all=True|Traceback|Error|no candidate" \
        | sed -e 's/^gen .* done in/GEN &/' -e 's/^.*Traceback.*/ERROR &/' -e 's/^.*VERIFIED.*/PASS &/' -e 's/^.*all=True.*/PASS &/'
      seen_out=$total
    fi
  fi
  # Progress of the evaluation log.
  lines=$(wc -l < "$LOG" 2>/dev/null || echo 0)
  now=$(date +%s)
  if [ "$lines" -gt "$last_lines" ]; then
    timeouts=$(tail -n +"$((last_lines + 1))" "$LOG" | grep -c '"deep_percent": 0.0, "spread": false, "sparse": false, "settles": false' || true)
    [ "$timeouts" -gt 0 ] && echo "TIMEOUT? $timeouts new evaluations failed every Stage 0 check (crash, timeout, or dead variant)"
    last_lines=$lines
    last_change=$now
  elif [ $((now - last_change)) -ge $((STALL_MIN * 60)) ]; then
    running=$(tasklist 2>/dev/null | grep -ci cosmos_x1 || true)
    if [ "$running" -eq 0 ]; then
      echo "DEAD no cosmos_x1.exe running and no new evaluations for $STALL_MIN min"
    else
      echo "STALL no new evaluations for $STALL_MIN min ($running runs still active)"
    fi
    last_change=$now
  fi
  sleep "$INTERVAL"
done
