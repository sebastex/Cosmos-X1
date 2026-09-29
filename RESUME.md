# Resume point (paused by user 2026-09-30 ~00:15)

## Main build (commit b798ffb, "version B"): streamed-word memory adopted
Full suite: SMALL seeds 22-27 ALL 3/6 (continual 3/6 weakest); DEV seeds 22-24 ALL 3/3 (every check).

## Candidate fix for small-brain interference: "combo" (NOT yet adopted)
Settings on top of main: inhibition_radius3=3, winners3=8, istdp_rate=200  (tools/combo_variant.json)
- Cause of interference (--test interfere): during a held cue a strong other memory co-activates
  (local inhibition -> no competition) or the cued memory tires under fast inhibition learning and flips.
- Fair 8-seed comparison (tools/results/fair.txt, seeds 22-29): combo continual 8/8, capacity 8/8,
  completion 4/4; main 4/8, 7/8, 4/4; each part alone worse.
- Combo completion 20/20 seeds (22-41), min +0.052 (tools/results/combo_comp.txt).
- Combo full suite small, fresh seeds 30-35 (tools/results/combo_small.txt): ALL 3/6; failures =
  input-driven reliability side check (0.41/0.47 < 0.5) on 33/34, CP6b margin lost 0.065-0.117
  (all old memories still correct) on 33/34/35, CP6 34 forgetting 0.051.
- Main build on the same seeds 30-35 (partial, tools/results/main_small_30.txt): seeds 30,31 fail
  capacity + retention (combo passed everything on 30,31).

## To do tomorrow (stopped mid-run)
1. Finish main build on small seeds 32-35 (same-seed comparison):
   python tools/baseline_eval.py --variants @tools/main_variant.json --seeds 32,33,34,35 --preset small --parallel 2 --threads 4
2. Combo on dev fresh seeds 25-27:
   python tools/baseline_eval.py --variants @tools/combo_variant.json --seeds 25,26,27 --preset dev --parallel 1 --threads 4
3. If combo >= main everywhere -> adopt into default rule (src/Config.cpp evolvedRule), rebuild, commit.
4. Check input-driven reliability drop with wider inhibition (side check 0.41-0.47).
5. Open user decision: CP6b / CP6 absolute margin-loss bar (0.05) vs "all old memories still correct".
