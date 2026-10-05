# Cosmos X1: Neural Cellular Matrix (NCM)

## Cosmos Prime (main version, updated 2026-10-05)

**Cosmos Prime** is the main version: the default rule in `src/Config.cpp`, git tag `cosmos-prime`.
It is the best version measured so far on the big brain (dev preset):

- Memory foundation ("version B"): 64 learnable long-range partners per voxel, anti-hub learning,
  strong learned links kept in check by gain control that adapts only with input (learned
  inhibition is built but switched off since evolution 2).
- Rhythm restart (`clock_reset`): the 2D/3D clocks restart at word gaps and onsets, so a word is
  cut into the same chunks every time.
- Front-to-back sweep (`field_sweep`): the four fields update in order within each step.
- Recency-weighted lines (`line_recency`, now 0.48): the newest letters count most; full strength kept.
- Two-route memory (`separation`, `spread_plastic`; added 2026-10-01 at 3 / 0.5, now 4.9 / 0.45): while a memory is
  stored, cells already loaded with memories are harder to recruit (pattern separation), and the
  feedforward sources from earlier fields learn the route from a cue to the stored cells (recall).
  On fresh dev brains 2002-2011: all checks 67/70 (before 65/70), brains passing every check 8/10
  (before 6/10), keeping old memories while learning new 9/10 (before 7/10).
- Evolution 2 tuning (winner c332, adopted 2026-10-04): more winners per voxel (5) with fewer per
  channel (2), slower learning (0.022) with a larger budget (5.7), stronger separation (4.9),
  partial covariance (0.58), shorter mode/order memory. Fair check on fresh dev brains 3000-3009:
  64 words truly recalled 73% (previous Prime 37%, runner-up c241 53%); checks 68/70 (same as before;
  one capacity and one continual miss, old memories kept).
- Per-link bounds (`link_bound` 0.3, replacing the channel-wide soft bound, adopted 2026-10-04): each link has its own
  room, so new memories are written at full strength. Fresh dev brains 3000-3009: checks 70/70;
  32 words 100%, 64 words 87% truly recalled (was 71%).
- Quiet ears (`silence_gate` 0.6, adopted 2026-10-05): without input the fixed paths (the echo
  of what was heard) turn down while learned links stay full, so the recalled partner wins over
  the cue's echo. Checks 70/70; 128 words truly recalled 72% (was 61%) on fresh dev brains.

Measured earlier (true recall of the partner word, `--test pairload`, seeds 22-31): 16 words 91%,
32 words 68% (the previous main version was at chance); an untrained twin stays at chance.
Official check: `--test suite` (CP2 recall, CP3 capacity, CP4 efficiency, CP5w word pairs, CP6,
CP6b, CP7). Build: `build.ps1` (Windows) or `build.sh` (Linux) -> `build/cosmos_x1(.exe)`.
All tests: `cosmos_x1 --help`. Diagnostics: `--test lookalike` (are look-alike words kept apart?),
`--test pairload` with `NCM_PAIR_FAST=1` (big loads) and `NCM_ECHO=1` (how much of recall is the cue's echo).


Test environment for Cosmos X1, an AI built on cellular-automaton principles: four 3D fields (Input, Memory, Reasoning, Output), every voxel holding a 2D sheet, every sheet cell holding a 1D line. Cells behave like neurons; knowledge is meant to emerge from local rules and local learning.

The specification lives in OneDrive: `Dokumenter\Cosmos X1\Neural cellular automata.md`. Section numbers in code comments refer to it.

## Status

| Stage | State |
|---|---|
| 0. Skeleton | Done: ladder, four fields, cell rule, local competition, homeostasis, position-weighted coupling, 4D link, long-range links, golden-ratio clocks, character fingerprints, sensory and motor surfaces, viewer |
| 1. Memory | Next: Hebbian learning of the 3D, long-range and 4D connections |

## Build

Needs g++ with C++20 and OpenMP (MinGW works):

```powershell
.\build.ps1
```

`CMakeLists.txt` is provided for machines with CMake.

## Run

```powershell
.\build\cosmos_x1.exe --preset tiny            # 8^3 fields, ~2 M cells, seconds
.\build\cosmos_x1.exe --preset dev             # 16^3 fields, ~18 M cells, about a minute
.\build\cosmos_x1.exe --preset full            # 32^3 fields, spec size; needs the 32 GB machine
```

Options: `--ticks N`, `--input-ticks N`, `--snap N`, `--out DIR`, `--text "..."`, `--seed N`, `--quiet`, `--set name=value` (tunable rule parameters; `--help` lists them).

Each run streams text into the sensory surface, then runs in silence, prints per-level activity, writes snapshots to `out\`, and ends with the Stage 0 check. The exit code is 0 when every check passes.

Snapshots are BMP images. Columns are the four fields; rows show the middle z-slice at the 3D, 2D and 1D levels.

## Stage 0 check

A run passes when the pattern:

1. reaches the Input field first;
2. spreads to Memory, Reasoning and Output;
3. stays sparse (bounded mean, some cells strongly on);
4. settles once input stops: bounded, still alive, and not frozen into a fixed pattern (homeostasis holds a quiet background near the activity target).

## Layout

```
include/ncm/   Config, Buffer, Random, Matrix, Scheduler, CharacterCodebook, Viewer
src/           Matrix.cpp (the cell rule and the three level steps), Config.cpp,
               CharacterCodebook.cpp, Viewer.cpp, main.cpp (Stage 0 run and check)
```
