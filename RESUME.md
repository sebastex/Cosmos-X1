# Resume point (2026-10-01 evening, user break)

## Main version: Cosmos Prime (git tag cosmos-prime, branch master)
Default rule = version B memory + clock_reset + field_sweep + line_recency 0.6. Unchanged since.

## What we know (evidence in tools/results/)
- evolution_2026-10-01/FINDINGS.md: 120 brain tests, 44 versions. Main failures: retention 50/120,
  continual 34/120, capacity 24/120; cause = old memories pushed aside by newer similar ones
  (interference 19 of 22), not erased; no setting fixes it (|r| <= 0.19) -> a missing part.
  Prime is among the best versions.
- separation_2026-10-01/: first pattern separation (branch `separation`, option `separation`):
  Prime 12/15, separation 3 -> 11/15, separation 8 -> 10/15 (capacity worse: stored pattern moved
  away from where the cue drives). Not adopted.
- 64-word diagnosis (Prime, 32 pairs, seeds 2000/2001): recalled 16 / 14, stored but NOT recalled
  14 / 15, not stored 2 / 3 -> at load the bottleneck is RETRIEVAL, not storage.

## Priorities when we resume
1. Retrieval under load + interference: two-route separation (hippocampus-like): separate similar
   memories at storage AND learn a strong cue -> stored-memory shortcut (learned 4D links / a
   dedicated path), so recall finds the separated memory. Test with tools/quick_suite.py and
   GitHub (tests.yml --ref <branch>, evolve.yml with monitoring).
2. Let the inner levels (sheets, lines) store memories.
3. Growth on demand (plan: keep wiring/indices, extend letter codes; cancelled for now by user).
4. Speed: learning step re-sums all links 2-3 times per active voxel; keep running totals (~2x).
5. Housekeeping: actions/checkout@v4 etc. -> Node 24 versions; update the spec document.

## Tools
tools/quick_suite.py (laptop, checks side by side), tests.yml (one check on many seeds),
evolve.yml + tools/evo (evolution with monitoring; settings.json hours), diagnostics:
recalldetail, pairload (per-pair stored/recalled), health, hum, interfere, wordcontext, ...
GitHub repo sebastex/Cosmos-X1 is PUBLIC (free Actions minutes).
