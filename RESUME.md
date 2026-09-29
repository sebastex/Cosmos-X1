# Resume point (2026-09-29, paused by user)

## Where we are: Stage 1, word completion (CP5b) vs memory capacity (CP3)
- Root cause of weak word completion: learnable links too sparse for scattered sparse codes
  (each active cell had ~1 learnable partner in the next letter's cells). Fix: long_range_links=64
  (long_range=0.00205 keeps fixed drive equal) + anti-hub rules (presynaptic_bound=1, covariance=1).
- Completion also needs strong links (plastic_budget=2, learning_rate=0.02): 20/20 seeds pass
  (seeds 22-41, small). Without strong links: 0/20.
- But strong links blur 8-memory recall (CP3 s22 gain -0.269) and break CP2/CP6/CP6b.
- `--test health` diagnostic (new) found: learned input 80-88% of drive (drowns cue), gain control
  only turns down input path (gain 0.25), attractors never switch off, hub cells (top1% = 45%).
- New options (default off): agc_plastic=1 (gain control also turns down learned input),
  depression_use / depression_tau (short-term synaptic depression of learned links).
  These fix hubs and blur (CP3 up to +0.324) but weaken completion. Trade-off not yet solved.
- Best so far (full = 64 links + antihub + budget 2 / rate .02):
    t.3/60 b3 (depression_use .3, tau 60, budget 3): capa22 +0.324 PASS, comp22 .048x comp23 .085 comp33 .021x
    t.2/40 b3: capa22 +0.065 (1 miss), comp 22/23 pass, comp33 .048x
- Remaining symptom: low "hum" of activity after recall never reaches zero -> bleeds into next cue;
  m->t miss on s22.
- Tools: tools/fixgrid.py (VARIANTS json env, TESTS env), tools/cgrid.py. Use build/cosmos_x1_diag.exe.
- Default rule unchanged (all new options off). Nothing adopted yet.

## Next ideas
- Resolve completion-vs-capacity: e.g. depression only in encoding-free recall, or completion read
  over a shorter window; check the "hum" floor source; then multi-seed CP2-CP7 + CP5b at small+dev.
