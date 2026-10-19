# Design: inner-level learning

Status: proposal for review (2026-10-14). Nothing here is built yet.

## Problem

Every voxel holds a sheet (64 cells x 8 channels = 512 values), and every sheet cell holds a line
(16 cells x 4 channels). Today only the voxel level learns:

- All memory links connect voxels to voxels (16 channels each, about 0.4% of all units).
- A sheet's 512 values reach its voxel through a **fixed random** projection (U2) and are squeezed
  into 16 channels. Two words with different sheet patterns but the same 16-channel summary are
  the same word to memory.
- In the deep fields the sheet is driven only by its own voxel (fixed random D2), so the inner
  pattern is an echo of the 16 channels and adds no information.

Measured consequences: the same 2 of 16 channels win in a voxel for about half of all words
(favourite channels), unrelated words overlap 9% instead of about 3%, and the same word is only
30-40% alike with itself in different surroundings.

## Principle

Each level learns the combinations it actually sees, with the same local rule the voxel level
uses (strengthen what fires together, bounded), and learns them **from its own parent and
children**, not from random partners. That was why the earlier inner-learning attempts failed:
their links went to random partners and carried mostly noise.

Every learned connection stays **fixed scaffold + plastic part**, as all learned links in Cosmos
are, so the spec's fixed position weights (2B) remain the starting wiring.

## Mechanisms (built and tested one at a time)

### 1. Learned upward path (sheet -> voxel)

Each voxel gets its own plastic weights from its 64 sheet cells (8 channels each) to its 16
channels, added to the fixed U2 projection.

- Rule: when channel c of the voxel fires, its weights from the sheet cells that are active
  strengthen; each channel's total upward weight is held constant (competitive learning).
- Effect: the 16 channels turn into detectors for the sheet patterns that really occur. Different
  inner patterns then drive different channels, so inner uniqueness becomes voxel-level
  uniqueness. Competition for a fixed total also stops favourite channels, because a channel that
  wins everything spreads its weight thin and loses.
- Consistency: what repeats every time a word is heard is strengthened every time; what changes
  with the surroundings is not. The detectors settle on the stable part of each word.
- Cost: 16 x 512 values per voxel (about 0.5 GB on the dev brain).

### 2. Learned downward path (voxel -> sheet)

Each voxel gets plastic weights from its 16 channels to its 64 sheet cells, added to the fixed D2.

- Rule: when a voxel channel and a sheet cell fire together, the link between them strengthens
  (bounded, with its own budget).
- Effect: when memory brings a voxel pattern back (recall), the voxel restores the detailed sheet
  pattern it had when the word was stored, which then drives the voxel back up through path 1.
  Voxel and sheet hold the memory together and sharpen each other, so the inner level stores the
  detail of every word instead of an echo.
- Cost: 64 x 8 x 16 values per voxel (about 0.5 GB).

### 3. Sheet links (within a sheet), later

Each sheet cell learns from its 8 neighbours (as the earlier sheet_links option did). Only worth
trying once paths 1 and 2 give the deep sheets real content.

### 4. Line level, later

The lines carry each letter's sequence. Learned line -> sheet weights (U1, the same rule as path 1)
would let sheets learn letter-sequence detectors. Only after 1-3.

## Drift and the critical period

Learning detectors changes which channels answer a word, and changing codes break links stored
earlier (channel balancing showed this). So the inner paths learn **fast early and then slow
down**, a critical period: their learning rate falls with how much each voxel has already
learned. This matches the curriculum: the inner levels settle their detectors during stage 1
(letters), and stay nearly still while vocabulary is stored in stage 2.

## What is not changed

Local rules only: every weight changes from what its own two ends do. No central check of
uniqueness or of patterns. Voxel-to-voxel links stay where memories between cells are stored.

## How it is judged (on GitHub, same brains as Prime)

1. Word overlap test: overlap between unrelated words (9% today) and favourite channels (53%).
2. Consistency: the same word in different surroundings (30-40% alike today).
3. Vocabulary test: completion from the first half, recognition, identity, at 300 / 600 / 1,200
   words.
4. Guards: alphabet and word-pair tests must not get worse.

Each mechanism goes into Cosmos Prime only if it beats Prime on these, on several fresh brains.
Order: 1 alone, then 1 + 2, then 3, then 4.

## Cost

Paths 1 + 2 add about 1 GB to the dev brain (about 2 GB to the mid brain, still under the 8 GB
base) and some time per step, mostly in active voxels.

---

# Target architecture: one homogeneous nested brain (4D)

## Guiding principle (the user's): as above, so below

- The structure above is the same structure as below: every level is built from the same parts
  and follows the same rules.
- What is above represents what is below to its fullest capacity: a cell's expression includes
  everything inside it. A 4D cell's expression is its whole content (every voxel, sheet and line),
  so it is far richer than any cell below it; nothing inside is compressed away.
- Function is not localized: a memory or an action happens wherever there is room at that moment,
  as a combination across all levels. Asking something triggers many patterns at once, in parallel.
- No scanning or parsing: recall is the whole system settling on the pattern that fits
  (content-addressable, all cells computing at once), never a search through stored items.

Status: proposal for review (2026-10-14), agreed in direction with the user: no segmented parts
with different jobs (segments add latency between them); one system with the same rules at every
level, nested all the way down.

## Structure

    4D cell  ⊃  a 3D map of voxels
    voxel    ⊃  a 2D sheet of sheet cells
    sheet cell ⊃ a 1D line of line cells

| Level | Contains | Its expression | Clock (ticks per step) |
|---|---|---|---|
| 1D line cell | — | its own 4 channels | 1 |
| 2D sheet cell | a line (16 cells) | its 8 channels and its line | φ |
| 3D voxel | a sheet (8 x 8 cells) | its 16 channels and its sheet | φ² |
| 4D cell | a whole 3D graph of voxels (B x B x B) | **everything inside it**: every voxel, sheet and line | φ³ |

A 4D cell is not a larger voxel with a small state of its own. Its expression is the full
configuration of everything it contains, so a single 4D cell means far more than any cell below it.
There is no compressed summary at the 4D level: nothing inside is thrown away.

The 4D cells form a true mathematical 4D lattice (A x A x A x A): every 4D cell has four
coordinates (x, y, z, w) and the full 4D neighbourhood of 3^4 - 1 = 80 neighbours. Distances,
neighbourhoods and competition are computed in 4D; there is no 3D layout behind it and nothing is
visualised. Every level follows the same rule set:

1. Up and down inside a cell: inner levels feed the level that contains them and are driven by
   it (fixed scaffold + learned part), so a pattern recalled at one level restores its detail below.
2. Lateral links between neighbours at the same level (voxels: 26; sheet cells: 8; line cells: 2),
   learned with the same local rule (strengthen what fires together or just before, per-link
   bound, per-cell budget, order window).
3. Between 4D cells (80 neighbours in the 4D lattice), links act on their whole contents, content
   to content: each voxel in one 4D cell's graph couples to its counterpart in the neighbour's graph,
   and through them their sheets and lines. Today's 4D link already does this on a small scale (the
   four fields are coupled position by position, whole map to whole map); the target generalizes
   that chain of four into a true 4D lattice.
4. Competition and homeostasis within each level, per channel.
5. Each level runs 1/φ as fast as the level inside it.

## No boundaries, no latency between regions

The voxel space is continuous across 4D cells: a voxel's neighbours and long-range partners can sit
in any 4D cell. Fast interactions go directly voxel to voxel, as today; the 4D level adds a slower
integrating layer on top, it is not a gate between regions. There are no fields with assigned
roles: the sensory surface is one face of the whole lattice and the motor surface the opposite
face; what any region does must emerge from learning.

## What it should give

- One stable pattern per word: a 4D cell's contents settle on one configuration for a whole word
  (its coupling to its neighbours runs on the slower clock φ³) while single voxels change every 2-3
  letters.
- More diverse combinations: a memory is a combination at four levels at once (which 4D cells,
  which voxels inside them, which sheet cells, which line cells), so two memories that share cells
  at one level still differ below.
- Several things at once: different 4D cells can hold different patterns at the same moment, so
  separate streams (a word being heard, a word being recalled, a sentence being followed) can run
  in parallel and combine.

## Sizes and memory (voxels regrouped, not added)

| Brain | 4D lattice | Voxels per 4D cell | Voxels | Memory (estimate) |
|---|---|---|---|---|
| dev4 | 4 x 4 x 4 x 4 = 256 | 4 x 4 x 4 = 64 | 16,384 (same as dev) | about 5-6 GB |
| mid4 | 4 x 4 x 4 x 4 = 256 | 5 x 5 x 5 = 125 | 32,000 (same as mid) | about 8-9 GB |

The 4D level adds content-to-content couplings between neighbouring 4D cells, about the cost of
today's 4D link per voxel; the inner-level learning (paths 1 and 2 above) adds about 1 GB per 16k
voxels.

## Cosmos Pharmakon 0 (built 2026-10-14, branch `cosmos-pharmakon`, setting `lattice4d=1`)

The first build of this architecture, a reroll tested against Prime on the same brains and tests.

- The voxels form a 4 x 4 x 4 x 4 lattice of 4D cells; each 4D cell holds a B x B x B graph of
  voxels (B = field_dim / 4: dev 4, mid 5) with their sheets and lines. Same voxel count as Prime.
- Inside each w the voxel space is continuous (26 neighbours, competition), across 4D cells too.
- Every voxel is linked to its counterpart (same place inside) in each of its 4D cell's 80
  neighbours: fixed scaffold (`lattice_link` each) + learned block (with a slow part). This drive is
  computed on 4D steps (every phi 3D steps) and held in between; these links learn on 4D steps.
- Same rules everywhere: no feedforward/feedback difference, no front-to-back sweep, pattern
  separation and the learned cue route (random sources anywhere) in every voxel, long-range links
  anywhere, gain control per 4D cell. No Input-field depth sources (a role).
- Senses enter at one face (w = 0, x = 0), the motor exit is the far face (w = 3, x = last).
- Inner-level learning (paths 1 and 2) is the next step on top.

## What changed since Pharmakon 0 (measured, 2026-10-14 to 2026-10-19)

Each change came from a measurement, not a guess; the earlier "same rules everywhere" list above is
kept for the record, and these are the places where the working brain differs from it.

- **0.1** (`lattice_scramble=1`, input depth on in the w = 0 slice): words overlapped 0.14 instead of
  0.09 because the sensory input was not spread over the w = 0 slice (the same in Prime without its
  input depth); the w = 0 slice is the 3D face of the 4D lattice where the senses arrive, so the
  input spreads over it. Scrambled counterparts removed a striped grid.
- **0.2** (`lattice_sep0=0`): pattern separation in the slice where the senses arrive distorted what
  the brain heard and destroyed pair recall (44% -> 100% at 16 pairs on brain 3004 without it).
- **0.3** (`lattice_exc_only=1`, `lattice_link=0.124`, `lattice_back=0.0875`, `lattice_sweep=1`): far
  4D cells fired at 0.08-0.14 strength because inhibitory partners cancelled ~90% of the 4D signal and
  the rest arrived diluted over up to 80 partners. Long-range links are now excitatory only
  (inhibition stays local), outward links have Prime's strength, links back toward the senses are
  weaker (x0.0875, Prime's own ratio) and the slices update in order from the senses. This brings
  back a direction of flow from the senses; it is the same rule for every cell, but it is a
  difference between directions, so the "no feedforward/feedback difference" line above no longer
  holds. 30% stronger links tip the brain into self-sustained humming.
- Options built and measured but not in the default (see Config.hpp): microtubules in every cell
  (`microtubule`, with control modes `mt_mode`), roots (`downward_gain` x3 + `inner_up`/`inner_down`
  + `up_evidence`), the one-rule version of the inner paths (`inner_rules`), sheet learning and
  continuous sheets (`sheet_learn`, `sheet_continuous`), edge scaling of the 4D drive
  (`lattice_edge_norm`).
- Still open (independent review): the humming margin as learning grows; lattice edges (a corner
  4D cell has 15 of 80 partners; `lattice_edge_norm` is the candidate fix); magnets (a familiarity
  rule or sleep-like unlearning); whether the 4D surface carries information (decode words per
  slice); the line level does not learn yet; microtubule memory is never freed.

## Way there (each step measured against Prime on fresh brains; Prime stays the safe version)

1. Inner-level learning inside today's structure (paths 1 and 2 above).
2. Homogenize: remove the role differences between today's four fields (feed-forward only,
   separation only in deep fields, field-specific gains, input depth only in the Input field), so
   the same rules hold everywhere and the voxels can be regrouped into the 4D cells' inner levels.
3. Build the 4D lattice: regroup the voxels into 4D cells and couple neighbouring 4D cells content to
   content (80 neighbours, clock φ³).
4. Re-run the full test set (alphabet, vocabulary, pairs, sentences, emergence) and the capacity
   curve on dev4 and mid4.
