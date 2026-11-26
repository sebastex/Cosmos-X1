#!/usr/bin/env bash
# Waits on GitHub until at least N more A/B runs (tests.yml + quick.yml) have finished, or MAX minutes pass,
# then exits so the improvement loop can score the new batch. Usage: bash tools/ab/waitbatch.sh N MAX
N=${1:-8}; MAX=${2:-120}
count() { gh run list --workflow tests.yml --status completed --limit 400 --json databaseId -q length 2>/dev/null || echo 0; }
start=$(count); t=0
while [ $t -lt $MAX ]; do
  now=$(count)
  [ $((now - start)) -ge $N ] && { echo "batch: $((now - start)) new runs finished"; exit 0; }
  sleep 300; t=$((t + 5))
done
echo "timeout: $(( $(count) - start )) new runs finished"
