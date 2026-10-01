# Evolution around Cosmos Prime (GitHub Actions)

Written by `.github/workflows/evolve.yml` (one generation per run, restarts itself until the deadline):

- `monitor.md` - condition monitoring report, refreshed every generation: leaders, which checks fail,
  which memories are recalled as others, how much of a memory a partial cue brings back, crashes.
- `state.json` - population, every candidate's results on every brain, deadline.
- `log.jsonl` - one line per candidate per brain.
- `best.json` - the current leader's settings.

Start: `gh workflow run evolve.yml -R sebastex/Cosmos-X1 -f hours=12 -f restart=true`
Stop: cancel the running workflow in the Actions tab (the state is kept).
