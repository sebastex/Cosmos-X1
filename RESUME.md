# Resume point (2026-10-01 evening, user break)

## Main version: Cosmos Prime (git tag cosmos-prime, branch master)
Default rule = version B memory + clock_reset + field_sweep + line_recency 0.6
+ two-route memory: separation 3 + spread_plastic 0.5 (adopted 2026-10-01; 67/70 vs 65/70 on dev 2002-2011)
+ evolution 2 winner c332 (adopted 2026-10-04): fresh dev brains 3000-3009, checks 68/70,
  64 words truly recalled 93/128 = 73% (old Prime 47/128 = 37%, c241 68/128 = 53% with 70/70).
  Archive: tools/results/evolution2_2026-10-03. Learned inhibition (istdp) is now off.
+ per-link bounds link_bound 0.3, soft_bound 0 (adopted 2026-10-04): 70/70 checks, 32w 100%, 64w 87%.
  Built, off by default: budget_trim (worse), replay (stuck on first memory), sheet_links (no gain),
  sheet_far (testing). Next: magnet fix, whole-word code, recall settling.

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
0. DONE: two-route memory adopted (keeping old memories fixed on most brains).
1. Retrieval under load (64-word ceiling ~50% unchanged by two-route) + remaining interference: two-route separation (hippocampus-like): separate similar
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
