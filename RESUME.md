# Where to resume (paused 2026-09-28 ~00:25)

Nothing needs to be redone. Everything below is committed, and all search and evaluation results are saved in `tools/`.

## Current baseline
- **Default rule:** the evolved champion (`src/Config.cpp` `evolvedRule()`, `tools/champion.json`). `--rule starting` gives the old hand-set rule.
- **Champion, small preset, fresh seeds 21–40, full suite:**
  - Stage 0: 20/20
  - recall, capacity, efficiency, order: 18/20
  - continual: 13/20
  - streamed: 0/20
  - Reports: `tools/baseline_report_round1.json` (seeds 21–25, champion plus 4 additions) and the terminal output of the 15-seed run.
- **Failures 38/39** are a total collapse: subtractive fatigue silences the matrix under held input.

## In progress when paused
1. **`fatigue_divisive=1`** (committed option, off by default):
   - Fixes the collapse on seeds 38/39.
   - The 20-seed eval was stopped after 9 seeds; the partial result is in `tools/baseline_report_divisive_partial.json`.
   - Continual was worse on 21–25: 2/5, against 5/5 for the champion.
2. **`fatigue_divisive=2`** (hybrid: divisive while input is present, subtractive in silence; built, not yet committed at pause time — committed now):
   - s22 forgetting 0.021, s24 0.137, s38 fixed (recall +0.104).
   - Was about to compare full CP6 lines (champion vs hybrid, seeds 22/24) to see which CP6 sub-condition fails.
3. **Evolution search** (round 3) is paused. Resume with:
   `python tools/evolve.py --regime rate --resume --log evolve_round3_log.jsonl --seeds-per-gen 3 --preset small --population 12 --generations 30 --parallel 2 --immigrants 2 --rng 97`

## Open decision (user)
- **CP5 streamed words** cannot pass with the current design: sub-voxel detail has no learning.
- Option 1: give 2D sheet cells learnable connections (spec change).
- Option 2: move streamed-word memory to Stage 2.

## Next steps
1. Finish the fatigue fix (divisive vs hybrid), then run the 20-seed eval (`tools/baseline_eval.py`).
2. Adopt the fix into `evolvedRule()` if it beats the champion.
3. Fix continual interference (overlap).
4. Act on the CP5 decision.
5. Verify at the dev preset.
