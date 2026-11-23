#!/usr/bin/env bash
# Reading test (2026-11-23, word_shape): words are their own shapes; letters are their own. In reading-aloud
# lessons (read op) it sees a word's letters, then hears the word's shape. Afterwards:
#   hear  - it hears the shape and says it back (as five.sh)
#   read  - it sees only the letters: does the mouth show the word? (letters -> word, learnt, never told)
# Control: LESSON=say teaches the shapes alone (no letters), so reading should stay at chance (~1/100).
# Usage: SHUFFLE=1 ROUNDS=8 WORDS="..." bash tools/console/read.sh "name|--set word_shape=1 ..."
cd "$(dirname "$0")/../.."
EXE=build/cosmos_x1; [ -f build/cosmos_x1.exe ] && EXE=build/cosmos_x1.exe
WORDS="${WORDS:-same least red worker team}"
NEW="${NEW:-lamp river}"
ROUNDS="${ROUNDS:-8}"
LESSON="${LESSON:-read}"
LEX=$(${PY:-python} -c "import sys; sys.path.insert(0,'tools/quick'); import quick; print(','.join(quick.words(100)+'$NEW'.split()))")
L="lexicon:$LEX"; T="still:1"
for r in $(seq $ROUNDS); do
  PASS="$WORDS"
  [ -n "$SHUFFLE" ] && PASS=$(${PY:-python} -c "import random,sys; w=sys.argv[2:]; random.Random(int(sys.argv[1])).shuffle(w); print(' '.join(w))" $r $WORDS)
  for w in $PASS; do L="$L;$LESSON:$w:$((8 / ROUNDS)):40"; done
done
for w in $WORDS $NEW; do T="$T;rest:150;repeat:$w:8:30;rest:150;repeat:$w:8:30:read"; done
S="$L;$T"
for v in "$@"; do
  name="${v%%|*}"; args="${v#*|}"
  for seed in ${SEEDS:-3004 3005}; do
    mkdir -p console; NCM_SCRIPT="$S" ./$EXE --test script --preset dev --variant ${VARIANT:-prime} --seed $seed $args 2>/dev/null > "console/read_${name}_$seed.txt"
    ${PY:-python} - "console/read_${name}_$seed.txt" "$WORDS" "$NEW" "$name" "$seed" <<'EOF'
import sys, json
f, words, new, name, seed = sys.argv[1], sys.argv[2].split(), sys.argv[3].split(), sys.argv[4], sys.argv[5]
out = {}
for l in open(f):
    k = l.split(' ', 1)[0]
    if k in ('REPEAT', 'READ'): r = json.loads(l.split(' ', 1)[1]); out[(k, r['heard'])] = r
def score(k):
    rows = [out.get((k, w)) for w in words]; rows = [r for r in rows if r]
    said = sum(r['said'] == r['heard'] for r in rows)
    top = sum(r['plan'].split(' ')[0] == r['heard'] for r in rows)
    wrong = sum(bool(r['said']) and r['said'] != r['heard'] for r in rows)
    own = sorted(float(r['plan'].split('| own ')[1].split()[0]) if '| own ' in r['plan'] else 0.0 for r in rows)
    return f"said {said}/{len(rows)} top {top} wrong {wrong} own-median {own[len(own)//2] if own else 0:.3f}"
nw = '  '.join(f"{w}:{(out.get(('READ', w)) or {}).get('said') or '-'}" for w in new)
print(f"{name} seed {seed}: HEAR {score('REPEAT')}  |  READ {score('READ')}  | never-learnt read: {nw}", flush=True)
EOF
  done
done
