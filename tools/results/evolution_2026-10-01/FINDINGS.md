# Evolution 2026-10-01: findings (5 completed generations, 120 brain tests, 44 versions of Cosmos Prime)

Run: GitHub Actions, workflow `evolve.yml`, stopped by the user after 5 generations (about 3.5 h).
Data: `state.json` (every candidate on every brain), `log.jsonl` (one line per test), `monitor.md` (last report), `best.json`.

## What keeps failing (all 120 tests)
| check | failed | Prime failed (of 10) |
|---|---|---|
| keeping 8 old memories after 16 new (retention) | 50 | 2 |
| keeping old memories while learning new (continual) | 34 | 2 |
| holding 8 memories (capacity) | 24 | 0 |
| recall of 3 memories | 9 | 0 |
| learning quickly (efficiency) | 9 | 2 |
| word pairs (true recall) | 2 | 0 |
| word order | 1 | 0 |

## Why
- Old memories are not wiped out; they are pushed aside by newer, similar ones at recall:
  19 of 22 measured failures mainly interference, 3 erasure.
- Storage is never the problem: word pairs stored and recalled 532, stored but not recalled 44, not stored 0.
- Load ceiling: true word recall about 70-80% at 32 words and 36-41% at 64 words for every version tested.
- No setting moves the main failure: largest correlation with keeping old memories |r| = 0.19
  (strongest with the overall score: encoding_suppression +0.38). By the run's reading rule this
  points to a missing part, not missing tuning.
- Prime is among the best versions; the best leader (c11: plastic_budget 3.90, assembly_inhibition
  0.0006) is only marginally ahead over 10 brains (score 8.88 vs 8.64).
- Recurring confusion across versions: "smile" recalled as "candy".

## Conclusion
Missing part: pattern separation when memories are stored (similar experiences given more distinct
stored shapes, local rules only), so a new memory does not sit on top of a similar old one. Expected
to reduce interference (retention/continual/capacity failures) and raise the 64-word ceiling.
