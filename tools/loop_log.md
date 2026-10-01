# Improvement loop (started 2026-10-01, on the laptop, big brain = dev preset)

Rule: one fresh brain per round, full Stage 1 suite (CP2, CP3, CP4, CP5w word pairs, CP6, CP6b, CP7).
On a failure: diagnose (health / hum / interfere / pairload per-pair), make ONE targeted engine change,
re-test the failing brain; keep the change only if it fixes it without breaking an earlier passing brain.
On a pass: next fresh brain. Goal: every check on every brain.

Starting config (on top of the default rule, which already has clock_reset): field_sweep=1, line_recency=0.6

| Round | Brain | Config change | Result | Notes |
|---|---|---|---|---|
