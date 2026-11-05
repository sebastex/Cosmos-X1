#!/usr/bin/env bash
# The tuning layer: on this machine, find the fastest way to run the same brain. Every setting is
# timed on one fixed lesson and its brain fingerprint (NCM_HASH) is compared with the plain build's:
# a setting that changes even one number of the brain is never chosen.
#   bash tools/tune/tune.sh > tune.json
# Stage 1: how the threads run (count, pinning, waiting), on the plain build.
# Stage 2: how the program is built (link-time optimisation, profile-guided, clang), with the best threads.
# Stage 3: two brains at once on the machine, each with half the cores.
set -uo pipefail
cd "$(dirname "$0")/../.."
VARIANT=${VARIANT:-vav}
PRESET=${PRESET:-dev}
LESSON='say:apple:4:40;say:table:4:40;seq:red,apple,is,sweet:3:40;rest:60'
CPU=$(grep -m1 'model name' /proc/cpuinfo | cut -d: -f2 | sed 's/^ *//')
NP=$(nproc)
SRC="src/*.cpp"
FLAGS="-std=c++20 -O3 -march=native -fopenmp -I include"
mkdir -p build/tune
log() { echo "$*" >&2; }

build() { # name compiler extra-flags
  local name=$1 cc=$2; shift 2
  $cc $FLAGS "$@" $SRC -o build/tune/$name 2>build/tune/$name.err || { log "build $name failed"; return 1; }
}
# run EXE ENV... -> "seconds hash"
run() {
  local exe=$1; shift
  local t0 t1 out
  t0=$(date +%s.%N)
  out=$(env "$@" NCM_HASH=1 NCM_SCRIPT="$LESSON" ./build/tune/$exe --test script --preset $PRESET --variant $VARIANT --seed 3004 2>/dev/null)
  t1=$(date +%s.%N)
  echo "$(python3 -c "print(round($t1 - $t0, 2))") $(echo "$out" | grep -oE 'BRAINHASH [0-9a-f]+' | awk '{print $2}')"
}

rows=()
add() { rows+=("{\"stage\":\"$1\",\"build\":\"$2\",\"env\":\"$3\",\"seconds\":$4,\"hash\":\"$5\"}"); log "$1 $2 [$3] $4 s $5"; }

build plain g++ || exit 1
read -r REF_T REF_H <<< "$(run plain OMP_NUM_THREADS=$NP)"
add reference plain "OMP_NUM_THREADS=$NP" "$REF_T" "$REF_H"

# Stage 1: threads.
best_env="OMP_NUM_THREADS=$NP"; best_t=$REF_T
for th in $NP $((NP > 2 ? NP - 1 : NP)); do
  for bind in none close spread; do
    for wait in passive active; do
      e="OMP_NUM_THREADS=$th"
      [ "$bind" != none ] && e="$e OMP_PROC_BIND=$bind OMP_PLACES=cores"
      e="$e OMP_WAIT_POLICY=$wait"
      read -r t h <<< "$(run plain $e)"
      add threads plain "$e" "$t" "$h"
      if [ "$h" = "$REF_H" ] && python3 -c "import sys; sys.exit(0 if $t < $best_t else 1)"; then best_env=$e; best_t=$t; fi
    done
  done
done

# Stage 2: builds, with the best threads.
best_build=plain
build lto g++ -flto=auto && { read -r t h <<< "$(run lto $best_env)"; add build lto "$best_env" "$t" "$h"
  if [ "$h" = "$REF_H" ] && python3 -c "import sys; sys.exit(0 if $t < $best_t else 1)"; then best_build=lto; best_t=$t; fi; }
# Profile-guided: build with counters, run the lesson once, rebuild using what was counted.
if build pgo-gen g++ -fprofile-generate -fprofile-update=atomic -fprofile-dir=build/tune/prof; then
  run pgo-gen $best_env > /dev/null
  for kind in pgo pgo-lto; do
    extra="-fprofile-use -fprofile-dir=build/tune/prof -fprofile-partial-training -Wno-missing-profile"
    [ $kind = pgo-lto ] && extra="$extra -flto=auto"
    build $kind g++ $extra && { read -r t h <<< "$(run $kind $best_env)"; add build $kind "$best_env" "$t" "$h"
      if [ "$h" = "$REF_H" ] && python3 -c "import sys; sys.exit(0 if $t < $best_t else 1)"; then best_build=$kind; best_t=$t; fi; }
  done
fi
if command -v clang++ > /dev/null; then
  build clang clang++ && { read -r t h <<< "$(run clang $best_env)"; add build clang "$best_env" "$t" "$h"
    if [ "$h" = "$REF_H" ] && python3 -c "import sys; sys.exit(0 if $t < $best_t else 1)"; then best_build=clang; best_t=$t; fi; }
fi

# Stage 3: two brains at once, half the cores each (throughput per brain = wall time / 2).
half=$(( NP / 2 > 0 ? NP / 2 : 1 ))
t0=$(date +%s.%N)
run $best_build OMP_NUM_THREADS=$half > build/tune/two_a.txt &
run $best_build OMP_NUM_THREADS=$half > build/tune/two_b.txt &
wait
t1=$(date +%s.%N)
two=$(python3 -c "print(round(($t1 - $t0) / 2, 2))")
h2=$(awk '{print $2}' build/tune/two_a.txt)
add two_brains $best_build "OMP_NUM_THREADS=$half x2" "$two" "$h2"

python3 - "$CPU" "$NP" "$REF_T" "$best_build" "$best_env" "$best_t" "$two" "$h2" "$REF_H" <<EOF
import json, sys
cpu, np_, ref, bb, be, bt, two, h2, refh = sys.argv[1:10]
rows = [json.loads(r) for r in '''$(printf '%s\n' "${rows[@]}")'''.strip().splitlines()]
print(json.dumps(dict(cpu=cpu, cores=int(np_), reference_seconds=float(ref), reference_hash=refh,
                      best=dict(build=bb, env=be, seconds=float(bt), speedup=round(float(ref) / float(bt), 3)),
                      two_brains=dict(seconds_per_brain=float(two), same_brain=h2 == refh,
                                      speedup=round(float(ref) / float(two), 3)),
                      rows=rows), indent=1))
EOF
