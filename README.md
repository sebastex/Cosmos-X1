# Cosmos X1: Neural Cellular Matrix (NCM)

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
