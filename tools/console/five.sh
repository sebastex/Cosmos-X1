#!/usr/bin/env bash
# Five-word repeat test (2026-11-22): a fresh brain learns 5 words (8 times each), then hears each word
# and says it back. Prints one line per version and brain: whole words right, and what it said.
# Usage: bash tools/console/five.sh "name|--set a=1 --set b=2" "name2|..."   (seeds 3004 3005)
# On Windows the compiler's DLL folder must be on PATH (mingw bin).
cd "$(dirname "$0")/../.."
EXE=build/cosmos_x1; [ -f build/cosmos_x1.exe ] && EXE=build/cosmos_x1.exe
WORDS="${WORDS:-same least red worker team}"
NEW="${NEW:-lamp river}"   # never learnt: can it say back a word it has never met?
# word_shape: the 100 ladder words (+ the new ones) are words it could say, so a lucky pick is ~1 in 100
LEX=$(${PY:-python} -c "import sys; sys.path.insert(0,'tools/quick'); import quick; print(','.join(quick.words(100)+'$NEW'.split()))")
L="lexicon:$LEX"; T="still:1"
if [ -n "$ROUNDS" ]; then  # spread practice: ROUNDS passes through the list, each word heard 8/ROUNDS times per pass
  for r in $(seq $ROUNDS); do
    PASS="$WORDS"  # SHUFFLE=1: a new order every pass (fixed per pass), so no word is always early
    [ -n "$SHUFFLE" ] && PASS=$(${PY:-python} -c "import random,sys; w=sys.argv[2:]; random.Random(int(sys.argv[1])).shuffle(w); print(' '.join(w))" $r $WORDS)
    for w in $PASS; do L="$L;say:$w:${PER:-$((8 / ROUNDS))}:40"; done
  done
  for w in $WORDS; do T="$T;rest:150;repeat:$w"; done
else
  for w in $WORDS; do L="$L;say:$w:8:40"; T="$T;rest:150;repeat:$w"; done
fi
for w in $NEW; do T="$T;rest:150;repeat:$w"; done
S="$L;$T"
for v in "$@"; do
  name="${v%%|*}"; args="${v#*|}"
  for seed in ${SEEDS:-3004 3005}; do
    mkdir -p console; NCM_SCRIPT="$S" ./$EXE --test script --preset dev --variant ${VARIANT:-prime} --seed $seed $args 2>/dev/null | tee "console/five_${name}_$seed.txt" |
      ${PY:-python} -c "
import sys, json
rows = [json.loads(l.split(' ', 1)[1]) for l in sys.stdin if l.startswith('REPEAT ')]
new = set('$NEW'.split())
fresh = [r for r in rows if r['heard'] in new]
rows = [r for r in rows if r['heard'] not in new]
right = sum(r['said'] == r['heard'] for r in rows)
first = sum(bool(r['said']) and r['said'][0] == r['heard'][0] for r in rows)
said = '  '.join(f\"{r['heard']}>{r['said'] or '-'}\" for r in rows)
nsaid = '  '.join(f\"{r['heard']}>{r['said'] or '-'}\" for r in fresh)
print(f'$name seed $seed: right {right}/{len(rows)}  first {first}/{len(rows)}  | {said}  || never learnt: {nsaid}', flush=True)
"
  done
done
