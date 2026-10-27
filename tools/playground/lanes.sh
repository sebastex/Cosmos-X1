#!/usr/bin/env bash
# Start one playground lane per Pharmakon variant (20 lanes; GitHub runs at most 20 jobs at once).
#   bash tools/playground/lanes.sh [RUNS] [SEED]      e.g. bash tools/playground/lanes.sh 8 3004
RUNS=${1:-8}
SEED=${2:-3004}
for v in alpha beta gamma delta epsilon zeta eta theta iota kappa lambda mu nu xi omicron pi rho sigma tau upsilon; do
  gh workflow run playground.yml -R sebastex/Cosmos-X1 --ref cosmos-pharmakon -f variant=$v -f seed=$SEED -f runs=$RUNS
done
