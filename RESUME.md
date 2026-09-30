# Resume point (paused by user 2026-09-30 ~13:00, "stop everything take a break")

## Main build = commit b798ffb rule ("version B"), unchanged since.
- DEV (bigger brain, now the pass mark; small = stress test only): full suite ALL pass on
  seeds 22,23,24,25,26 (5/5). Seed 27 was stopped mid-run -> rerun:
  python tools/baseline_eval.py --variants @tools/main_variant.json --seeds 27 --preset dev --parallel 1 --threads 5
- SMALL: 12 seeds (22-27, 30-35): capacity 7/12, continual 7/12, retention 7/12, recall 9/12, rest 12/12.

## "combo" interference fix (inhibition_radius3=3, winners3=8, istdp_rate=200): NOT adopted.
Helps small (capacity 6/6, ALL 3/6 vs 0/6 on seeds 30-35) but at dev: retention 1/3 (seeds 25-27),
streamed gain halved. Main passes all on dev 25, 26. Keep as option.

## New finding: many-words limit (both sizes)
New checks: --test wordcapacity (8 words), wordcontinual (3+3 words), wordcontinualswap.
- Benefit of learning per word ~ 1/number of words (3: +0.09, 6: +0.045, 8: +0.03).
- Cause 1 (--test wordshape): no field holds a steady shape for a word; a word is a trail over
  ~11% of cells (1% per moment) -> unrelated words ~11-17% alike by chance.
- Cause 2: learning brakes (soft_bound, presynaptic_bound) weaken later words.
- New option field_pace (deeper fields integrate slower; 1.618 works, 2.5 breaks learning).
- Best so far: field_pace=1.618 + soft_bound=0.25.
  SMALL (22,23): wordcontinual pass both; wordcapacity gain +0.083/+0.061 (1 word wrong each);
    letter continual s25 lost 0.338 (fail), s23 pass; capacity s23 pass.
  DEV (tools/results/pace_dev.txt): wordcontinual s22 pass, s23 old lost 0.126 (fail);
    wordcapacity s22 +0.131 PASS, s23 +0.106 (1 word wrong); completion s22 +0.244 (main ~0.11);
    efficiency ok; BUT letter continual fails: s22 lost 0.080, s23 lost 0.171 (main passes).
  -> words much better, old-letter protection worse. Not adopted. Next: keep word gain while
     protecting old memories (e.g. soft_bound between .25 and 1 at dev, or brake only per-cell).

## User decisions / preferences (2026-09-30)
- Bigger brain (dev) is the minimum size / pass mark; small brain = stress test.
- Memories are non-local shapes; give the engine parts, results must be learnt; black box is fine.
- Answers must be short and simple.
- Open: CP6/CP6b absolute 0.05 margin-loss bar vs "all old memories still correct".
- Open: is many-words a Stage 1 gate or first task of Stage 2?
- User asked about speed/hardware; brain should grow on demand, quality from learning not size.
