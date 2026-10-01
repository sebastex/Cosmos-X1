# Resume point (2026-10-01)

## Main build ("stem") = default rule in src/Config.cpp
Version B (64 long-range links, anti-hub learning, strong links, learned inhibition, hum fix, ...)
+ clock_reset=1 (rhythm restart at word gaps and onsets)
+ field_sweep=1 (front-to-back sweep of the four fields)
+ line_recency=0.6 (recency-weighted line summary, full strength)
Suite (`--test suite`): CP2 recall, CP3 capacity, CP4 efficiency, CP5w word pairs (honest), CP6, CP6b, CP7.

## Evidence (dev, big brain)
- True word-pair recall (pairload): 32 words 68%, 16 words 91% over seeds 22-31 (GitHub). Main before: chance.
- Held-letter capacity: passes on 25, 31 (which failed with sweep + line_carry 0.6).
- Full suite of this exact config: running in the improvement loop (tools/loop_log.md).

## Tools
- tools/quick_suite.py --seed N : the 7 checks side by side on one brain (laptop).
- GitHub Actions: gh workflow run tests.yml -R sebastex/Cosmos-X1 -f args="..." -f seeds="[..]" (minutes are limited).
- Diagnostics: health, hum, interfere, chain, pairlinks, wordcontext, wordshape, discriminate, pairload per-pair readout.

## Next
- Improvement loop on the laptop (user: run until told to stop; goal every check on every brain).
- Stronger recall: most missed words are stored but not recalled; suspect = learned links muted in the
  silence right after a cue (encoding_suppression=1, mode_tau 30) just when the partner should come out.
- Words drift from their own shape as more words are learned.
