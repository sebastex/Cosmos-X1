#pragma once
#include <cstddef>
#include <cstdint>
#include <string>

namespace ncm {

// The four 3D fields that make up the 4D level (spec Section 2A).
enum class Field : uint32_t { Input = 0, Memory = 1, Reasoning = 2, Output = 3 };
inline constexpr uint32_t kFields = 4;

inline constexpr double kPhi = 1.6180339887498949;

// Channels per cell are fixed by the architecture (spec Section 2A).
inline constexpr uint32_t C1 = 4;  // 1D line cell
inline constexpr uint32_t C2 = 8;  // 2D sheet cell
inline constexpr uint32_t C3 = 16; // 3D voxel

// Per-level cell behaviour (spec Sections 3C, 4A).
struct LevelParams {
    float homeostasis_rate; // threshold adaptation per tick (0 = off)
    float theta_min;        // 0 keeps quiet regions quiet: no cell can switch itself on
    float theta_max;
    float active_level;     // strongest-channel value at which a cell counts as active (statistics);
                            // for 1D motor readout, the mean channel value
    // Fatigue (spike-frequency adaptation): a cell that keeps firing tires, raising its
    // effective threshold, and recovers once silent, so old patterns switch off instead of
    // lingering. Unlike homeostasis it resets within tens of ticks.
    float fatigue_gain = 0.0f; // 0 = off
    float fatigue_tau = 20.0f; // ticks
};

// Hand-set starting rule. Stage 5 (rule evolution) replaces these values.
// Values from the Stage 0 and Stage 1 parameter searches. Recurrent loops stay below a
// gain of 1 so the matrix is input-driven with a fading memory: the same input gives the
// same pattern (the Stage 1 reliability check). Neighbour and long-range links are strong
// enough to carry input from the sensory face into the field's interior.
struct StartingRule {
    float line_carry      = 1.0f;  // 1D: weight from the left neighbour, carries sequences toward the exit
    float sheet_self      = 0.3f;  // 2D: persistence of a cell's own state
    float sheet_neighbour = 0.06f; // 2D: each of the 8 lateral neighbours
    float voxel_self      = 0.3f;  // 3D: persistence of a voxel's own state
    float voxel_neighbour = 0.1f;  // 3D: scaffold strength from each of the 26 neighbours
    float long_range      = 0.15f; // 3D: scaffold strength of each long-range link
    // 4D link scaffold, directional along the processing order Input -> Memory -> Reasoning ->
    // Output. Feedforward carries the signal; feedback is weaker, as in cortex. Loops between
    // fields have strength forward x backward, so a strong forward path stays loop-safe.
    // (Symmetric links strong enough to carry streamed text made fields sustain each other.)
    // Measured: forward 0.2-0.3 lets streamed text spread strongly but makes the deep fields
    // near-copies of the input, so learning adds nothing to recall; 0.1 keeps recall working.
    // Per-field gain control is the planned fix for the weak spread of streamed text at 0.1.
    float link4d          = 0.1f;  // forward: from an earlier field to a later one
    float link4d_backward = 0.1f;  // feedback: from a later field to an earlier one
    // With link4d_spread > 0: share of the feedforward scaffold strength carried by the random
    // sources (split evenly among them); the same-position source keeps the rest.
    float link4d_spread_share = 0.5f;
    // Multiplies the strength of each random 4D source (1 = the shared strength split evenly;
    // equal to link4d_spread = each source as strong as the whole same-position link, so a
    // target can fire from one or two active sources and competition keeps the strongest).
    float link4d_spread_gain = 1.0f;
    float input_depth_gain = 0.2f; // strength of each Input-field depth source
    // Pharmakon (lattice4d): fixed scaffold strength of each of a voxel's 80 4D-lattice links
    // (one per neighbouring 4D cell). About link4d / 9: a voxel near the sensory face then gets
    // about the drive Prime's feedforward 4D link gives it (about 9 active counterparts at once).
    float lattice_link = 0.0276f;
};

// Hebbian learning (spec Section 5A).
struct LearningParams {
    float rate = 0.2f;       // eta; scaled by the surprise modulator M(t)
    float order_gain = 0.5f; // lambda: weight of the order (j before i) term
    // Weight of Oja's normalization term. It shrinks all of a cell's plastic inputs whenever the
    // cell is active, which makes each cell keep only its most recent pattern (forgetting);
    // the synaptic-scaling cap already bounds strength.
    // Measured: no benefit, so off by default; the cap bounds strength instead.
    float oja = 0.0f;
    // Every learned connection = fixed scaffold + plastic part. Synaptic scaling caps each
    // output channel's total plastic input at this budget, so a cell's memories compete for
    // a fixed amount of strength while its scaffold (the basic wiring) is never touched.
    // 0.5: strong enough to complete patterns. (Larger budgets only let a memory take over
    // while fatigue was mis-measured; with fatigue fixed, 0.5 passes the Stage 1 test.)
    float plastic_budget = 0.5f;
    // Covariance learning: association uses activity above each cell's own long-run
    // average, so features active in every pattern stop being reinforced and what makes
    // each pattern distinct is stored. 0 = plain Hebbian association.
    // 0.5 (tuned at small size, 3 seeds): full covariance over-penalized shared features and
    // cost capacity on some seeds; with encoding suppression 0.95 it doubled the passes of the
    // borderline checks (4/9 vs 2/9) and nearly removed forgetting on 2 of 3 seeds.
    float covariance = 0.5f;
    float average_tau = 500.0f; // 3D ticks over which each cell's average activity is tracked
    // Encoding vs recall (the role acetylcholine plays in the brain): at modulator M, learned
    // connections transmit at (1 - encoding_suppression * M). New material is written without
    // being captured by existing memories; familiar input recalls at full strength.
    float encoding_suppression = 0.95f; // protects old memories while new ones are written
    // The same suppression applied to the learned part of the 4D link (its scaffold, the input
    // path between fields, is never suppressed). In the hippocampal model acetylcholine turns
    // down every learned associative pathway and spares only afferent input. 0 = off.
    float encoding_suppression_4d = 0.0f;
    // Learned competition between memories (assembly inhibition): each long-range link also has a
    // learned inhibitory weight. While memories are written, a partner that fires while this voxel
    // stays silent strengthens its inhibition onto the voxel (they belong to different memories),
    // and a partner that fires together with it weakens it (same memory). Recalled memories then
    // push competing ones down, so recall settles on one memory. Transmitted like other learned
    // links (muted while encoding). rate 0 = off; assembly_max caps each link's weight.
    float assembly_inhibition = 0.0f;
    float assembly_max = 0.02f;
    // Timing window of the order term (3D ticks): "j before i" counts j's decaying recent
    // activity, as in spike-timing-dependent plasticity, not only the previous step. Without
    // it an order link forms only in the one or two steps where one item hands over to the
    // next. 0 = previous step only.
    float order_tau = 0.0f;
    // Kinetics of the encoding/recall mode (1D ticks): suppression of memory circuits builds
    // up over mode_tau after input ends or turns novel; recall takes effect at once. Leaves a
    // window after a cue in which the next item of a learned sequence can play out. 0 = off.
    float mode_tau = 0.0f;
    // Heterosynaptic depression: how much an active cell weakens its inputs from silent cells
    // (covariance learning's depression term). Those silent inputs include the cells of other,
    // older memories, so full depression erodes them wherever memories share cells.
    // 1 = full covariance (default), 0 = only inputs that were active change.
    float hetero_ltd = 1.0f;
    // Presynaptic soft bound: strengthening is also scaled by 1 - presynaptic_bound * (share
    // of the source channel's outgoing budget already used). Cells wired into stored memories
    // then form new links slowly, so new memories recruit fresh cells (pattern separation).
    // 0 = off.
    float presynaptic_bound = 0.0f;
    // Consolidation (two-component synapses): each plastic connection has a fast part (learned
    // as above, freely overwritten by new learning) and a slow part that follows the fast part
    // upward at this rate per learning step and has its own budget. Old memories persist in the
    // slow part when new learning reshapes the fast part, without slowing new learning.
    // 0 = off (no slow part).
    float consolidation_rate = 0.0f;
    float consolidated_budget = 0.5f; // per output channel, like plastic_budget
    // Per-link bound: each link strengthens in proportion to its own room, 1 - w / link_bound,
    // so a new memory is written at full strength however full the channel already is (the
    // earlier channel-wide room wrote newer memories weaker). 0 = off.
    float link_bound = 0.0f;
    // Quiet ears (silence_gate): while no input arrives, each voxel turns its fixed input paths
    // (the fading trace of what was heard) down by silence_gate and keeps its learned links at
    // full strength, so after a word ends the cells follow what memory brings back (the
    // associate) instead of the word's own echo. 0 = off, 1 = fixed paths silent.
    float silence_gate = 0.0f;
    // Field roles: the Memory field holds vocabulary (each word on its own), the other fields hold
    // what happens between words. At every word boundary (a space, or input after a pause) the
    // Memory field's order window is cleared and its learning pauses for field_roles 3D steps, so
    // the end of one word is never tied to the start of the next there. 0 = off (every field learns
    // across words).
    float field_roles = 0.0f;
    // Channel balance: homeostasis for each channel of a voxel, not only for the voxel as a whole.
    // A channel's own threshold offset rises by channel_balance x (its output - the voxel's mean
    // channel output) whenever the voxel fires, so channels that win for every input (because
    // their fixed random wiring happens to be stronger) get harder to win and the others easier.
    // The offsets of a voxel sum to zero, so the voxel's own activity is unchanged. Without it
    // the same 2 channels carried about half a voxel's activity over all words. 0 = off.
    float channel_balance = 0.0f;
    // Inner-level learning (docs/design-inner-level-learning.md). Path 1, learned upward
    // (sheet -> voxel): each voxel channel learns, competitively, the sheet pattern present when
    // it fires (w += r * out_c * (x / sum(x) - w), so each channel's weights stay a normalised
    // pattern summing to at most 1); it adds inner_up x upward_gain x (w . sheet) to the channel.
    // Path 2, learned downward (voxel -> sheet): each sheet cell learns, per voxel channel, the
    // state it had when that channel fired (outstar: w += r * voxel_c * (sheet - w)); it adds
    // inner_down x downward_gain x (w . voxel) to the sheet cell, so a recalled voxel restores
    // its detail below. Rate r = inner_rate x M, slowed by the critical period: divided by
    // (1 + learned / inner_period), where learned is the voxel's accumulated firing while
    // learning (0 = no slowdown). 0 gains = off.
    float inner_up = 0.0f;
    float inner_down = 0.0f;
    float inner_rate = 0.05f;
    float inner_period = 0.0f;
    // 2D level learns (every level influences its neighbours): each sheet cell gets a learned
    // C2 x C2 block from each of its 8 neighbours on top of the fixed rule, learned on every 2D
    // step with the voxels' rule (fire together / in order, Oja, per-link bound sheet_link_bound,
    // each channel's learned input capped at sheet_budget). Muted while encoding like the
    // voxels' learned links. Rate = sheet_learn x M; 0 = off.
    float sheet_learn = 0.0f;
    float sheet_budget = 1.0f;
    float sheet_link_bound = 0.3f;
    float modulation_rate = 1.0f;   // learning-rate multiplier for each voxel's sheet modulation (0 = off)
};

struct Config {
    // Geometry (spec Section 2A). The spec's full size is 32 / 16 / 16.
    uint32_t field_dim        = 16;
    uint32_t sheet_dim        = 8;
    uint32_t line_len         = 16;
    uint32_t long_range_links = 4;
    // Seed of the items (character fingerprints and cue thinning); 0 = the matrix seed. Lets
    // tests vary the items and the matrix's random wiring independently.
    uint32_t codebook_seed = 0;
    // Pattern separation on the feedforward 4D link (dentate-gyrus-like): besides the voxel
    // at the same position, each voxel receives fixed scaffold input from this many random
    // positions of every earlier field. Similar inputs (which overlap position by position)
    // then drive different combinations of voxels, and local competition keeps the most
    // strongly driven, so their representations overlap less. 0 = same position only.
    uint32_t link4d_spread = 0;
    // Input-field depth: random sensory-face sources per interior Input voxel (0 = off).
    uint32_t input_depth_spread = 0;
    // 1 = the number of random 4D sources scales with the field side (link4d_spread at side 12).
    float link4d_spread_scaled = 0.0f;

    float target_activity  = 0.02f; // spec Section 3B
    float inhibitory_share = 0.20f; // spec Section 3C, applies to 2D and 3D cells
    // Inhibitory connections are stronger than excitatory ones so that 20% of cells can
    // balance the other 80% (balanced excitation and inhibition, as in cortex).
    float inhibitory_strength = 4.0f;
    // Local competition (spec Section 3C): a cell stays active only if fewer than this
    // many neighbours are driven harder. Above 1 so neighbouring cells can fire together.
    uint32_t winners2 = 1; // of 8 sheet neighbours (sheets do not learn, so strict competition)
    // Voxel competition reaches further than a voxel's own connections, as inhibition does in
    // cortex: within a (2R+1)^3 neighbourhood only the top `winners3` stay active. R = 2 with 3
    // winners gives ~2.4% activity (the spec's sparse target) while neighbours can still fire
    // together. Measured: with radius 1, ~15% of voxels were active and learning lost specificity.
    uint32_t inhibition_radius3 = 2;
    uint32_t winners3 = 3;
    // Competition inside a cell: a cell is a small group of neurons (its channels), and only
    // its most strongly driven channels stay active, so *which* channels fire depends on the
    // content. Without it the same cells win for every input and patterns overlap heavily.
    uint32_t channel_winners2 = 2; // of C2 = 8
    uint32_t channel_winners3 = 3; // of C3 = 16; sparser than 4 reduces overlap (cross-talk) between memories

    // Position weights are scaled by 1/sqrt(fan-in), so an upward gain of 1 keeps a
    // summary's strength roughly equal to what it summarizes (spec Section 2B).
    // Upward summary gains (line -> sheet, sheet -> voxel).
    float line_upward_gain = 1.0f;
    float upward_gain   = 1.0f;


    // Firing (all-or-none output): a cell that wins the competition and whose strongest
    // channel reaches this level fires at full strength (pattern kept, strongest channel 1);
    // below it the cell is silent. Signals then keep their strength from field to field
    // instead of shrinking at every step. 0 = off (graded output).
    float fire_threshold2 = 0.0f; // sheet cells
    float fire_threshold3 = 0.0f; // voxels
    // Rate coding instead of all-or-none: with a gain > 0, output = min(1, gain * (activity -
    // fire_threshold)) per channel. All-or-none firing plus fatigue made cells blink on and off
    // under steady input (unreliable patterns); a steep, saturating firing curve lets fatigue
    // lower the rate smoothly. 0 = off.
    float fire_gain2 = 0.0f;
    float fire_gain3 = 0.0f;

    // Per-field gain control: each 3D field scales the 4D link from earlier fields (its only
    // loop-free input) so its activity tracks the target. Faint streamed input is amplified,
    // strong input is turned down. Its own sheet summaries (a voxel -> sheet -> voxel loop),
    // internal loops and feedback from later fields are never scaled; amplifying any of them
    // made activity self-sustain. The Input field has no earlier field, so its gain is inert.
    // agc_rate = 0 turns it off.
    float agc_rate = 0.0f;  // per 3D tick, relative adjustment toward the target (off by default)
    float agc_min = 0.25f;
    float agc_max = 16.0f;
    float agc_relax_field = 0.0f; // field-wide gain control: relaxation toward 1 per 3D tick in silence
    // 1 = gain control below 1 also turns down the learned (recurrent) input, not only the input
    // path: when a field is too active because memories drive it, the memories are turned down
    // instead of the cue. Gain above 1 never amplifies learned input (loops would self-sustain).
    float agc_plastic = 0.0f;
    // 1 = gain control adapts only while the senses receive input; in silence every gain relaxes
    // toward 1 (at agc_relax_field). Otherwise a faint echo after the input ends is
    // taken for weak input, and the rising gain turns the echo into self-sustaining activity.
    float agc_input_only = 0.0f;
    // 1 = the 2D and 3D clocks restart their cycle after every word gap (space character) and
    // when input begins after a pause, so a word is cut into the same chunks every time it is
    // heard, wherever it appears.
    float clock_reset = 0.0f;
    // 1 = front-to-back sweep: on every 3D step the four fields are updated in order (Input,
    // Memory, Reasoning, Output) and each field reads the NEW state of the fields before it, so
    // the whole matrix looks at the same moment. 0 = all fields update from the previous step's
    // states, which delays every field by one more step (the Output field hears a word when the
    // next one is already arriving). Feedback from later fields still comes from the previous step.
    float field_sweep = 0.0f;
    // Cosmos Pharmakon: 1 = the voxels form one homogeneous 4D lattice instead of four fields
    // with roles. The brain is a 4 x 4 x 4 x 4 lattice of 4D cells (coordinates x, y, z, w); each
    // 4D cell contains a whole 3D graph of B x B x B voxels (B = field_dim / 4) with their sheets
    // and lines. Voxel space is continuous inside each w (a voxel's 26 neighbours may sit in the
    // next 4D cell). Every 4D cell is coupled content to content with its 80 neighbours: each voxel
    // to its counterpart (same place inside) in every neighbouring 4D cell, fixed scaffold +
    // learned part, on the 4D clock (every phi 3D steps). The same rules hold everywhere: no
    // feedforward/feedback difference, separation, cue route and long-range links everywhere,
    // gain control per 4D cell. Senses enter at one face (w = 0, x = 0), the motor exit is the far
    // face (w = 3, x = last). 0 = Cosmos Prime's four fields.
    float lattice4d = 0.0f;
    // Pharmakon: 1 = each 4D-lattice link joins a voxel to a scrambled counterpart: the place
    // inside the neighbouring 4D cell goes through a fixed random one-to-one shuffle (one per
    // direction, the opposite direction uses the reverse shuffle; the same for every 4D cell).
    // Still whole content to whole content, but a pattern is not copied to the same place in
    // every neighbour (that made the voxel space a striped grid). 0 = same place.
    float lattice_scramble = 0.0f;
    // Pharmakon: 1 = a voxel's random spread sources (the learned cue route) are drawn from its
    // own 4D cell and its 80 neighbours instead of anywhere, so near the senses every voxel hears
    // a random mix of the input (pattern separation everywhere, by one local rule). 0 = anywhere.
    float spread_local = 0.0f;
    // Pharmakon: 1 = the 4D-lattice drive is computed on 4D steps and held in between (the 4D
    // clock); 0 = it is computed on every 3D step.
    float lattice_hold = 1.0f;
    // Pharmakon: 1 = on each 3D step the w-slices update in order from the senses (w = 0) on, each
    // reading the new state of the slices before it (no extra step of lag per slice); 0 = all at once.
    float lattice_sweep = 0.0f;
    // Pharmakon: the region whose gain control acts together: 0 = each 4D cell, 1 = each w-slice
    // (Prime's grain: a quiet region raises its gain as a whole), 2 = the whole brain.
    float lattice_agc = 0.0f;
    // Pharmakon: the scaffold of a 4D-lattice link falls with 4D distance: a link to a 4D cell that
    // differs in d of its 4 coordinates has strength lattice_link x lattice_falloff^(d - 1), so the
    // 8 face neighbours couple most. 1 = all 80 links the same.
    float lattice_falloff = 1.0f;
    // Pharmakon: direction of flow, the same rule for every cell: a link's scaffold is scaled by
    // lattice_back when its source lies further from the senses (higher w: the signal flows back
    // toward where it entered) and by lattice_side when the source has the same w. Links whose
    // source lies nearer the senses keep full strength. 1 = no direction (all links equal).
    float lattice_back = 1.0f;
    float lattice_side = 1.0f;
    // Pharmakon, Prime's choices one at a time (diagnosis): lattice_lr_slice = 1: long-range
    // targets inside the voxel's own w-slice (Prime: inside its field); lattice_spread_back = 1:
    // spread sources only in slices nearer the senses (Prime: earlier fields; w = 0 has none);
    // lattice_sep0 = 0: no pattern separation in the w = 0 slice (Prime: none in the Input field).
    float lattice_lr_slice = 0.0f;
    float lattice_spread_back = 0.0f;
    float lattice_sep0 = 1.0f;
    // Pharmakon: 1 = links between 4D cells come only from excitatory voxels (as long-range
    // connections between brain areas are excitatory; inhibitory cells act only on their own
    // neighbourhood). The firing probe showed inhibitory 4D partners cancelling about 90% of the
    // excitatory 4D signal, so cells far from the senses fired at 0.08-0.14 strength. 0 = both.
    float lattice_exc_only = 0.0f;
    // Pharmakon: every 4D cell treated alike at the edges of the lattice. 1 = a voxel's 4D drive is
    // scaled up to the full 80 links (measured: pushes the brain over the humming edge); 2 = scaled to
    // the lattice's average number of links, so edge cells get more, middle cells less and the total
    // stays the same. 0 = edge cells simply get less.
    float lattice_edge_norm = 0.0f;
    // No islands at 2D: 1 = the sheets of neighbouring voxels join edge to edge into one
    // continuous sheet (the sheet of voxel (x, y) continues into the sheets of (x +- 1, y +- 1)
    // in the same z and w), so a sheet cell's 8 neighbours, its competition and its learned links
    // reach across voxels. 0 = each voxel's sheet is closed.
    float sheet_continuous = 0.0f;
    // Evidence only, the same rule at every level: a child reports up only the part of its drive
    // that did not come from its own parent. A sheet cell lit only by its voxel sends nothing
    // back to that voxel (its roots show, but cannot echo), while sheet cells lit from their lines
    // or neighbours count fully. Strength 0..1 (0 = off: the whole state goes up).
    float up_evidence = 0.0f;
    // One rule everywhere: 1 = the learned inner paths (inner_up, inner_down) behave like every
    // other learned link - muted while encoding (encoding_suppression), turned down with gain control
    // (agc_plastic), and not cut by the quiet-ears gate in silence (the gate turns down only the fixed
    // echo paths). 0 = they ride on the fixed paths (the first version).
    float inner_rules = 0.0f;
    // Familiarity (against magnets): before a voxel learns, its learned links predict its firing from
    // what fired just before (the same inputs the learning rule uses). The share of its current firing
    // that is already predicted, f in [0, 1], lowers its learning rate to rate x (1 - familiarity x f):
    // a well-known pattern stops strengthening (its basin stops growing), new patterns still learn
    // at full rate. A local rule, the same for every voxel. 0 = off.
    float familiarity = 0.0f;
    // Learn by predicting: 1 = the brain's surprise (which sets how strongly it learns and whether
    // it is in encoding or recall mode) is how much of its voxels' firing their learned links did NOT
    // predict from what fired just before (firing-weighted over all firing voxels), instead of the
    // untrained motor guess. Familiar input is then unsurprising: learned less, recalled more.
    float predict_surprise = 0.0f;
    // Microtubules (Orch-OR-like, local quantum mathematics): inside every cell a cylindrical
    // lattice of 13 protofilaments x mt_len tubulin units. Each unit holds a superposition
    // a0|0> + a1|1> (complex amplitudes) and evolves unitarily under H = mt_bias sz + h sx, where
    // h = mt_couple x (sum of its 4 neighbours' <sx>, helical seam included) + mt_drive x (its voxel
    // channel's activity): superposition, phase and interference. Each 3D step the microtubule
    // accumulates its superposition (sum of 4 |a0|^2 |a1|^2); when the total passes mt_threshold
    // it reduces objectively: every unit collapses to 0 or 1 with the Born probabilities. The
    // collapsed pattern shapes the cell's firing (mt_gain x share of each channel's units at 1 x
    // the cell's own mean positive input; it never creates activity), fading by mt_decay per step.
    // Every cell of the levels in mt_levels has one (src/Microtubule.cpp). 0 = off.
    float microtubule = 0.0f;
    float mt_len = 8.0f;
    float mt_couple = 0.3f;
    float mt_drive = 1.0f;
    float mt_bias = 0.2f;
    float mt_threshold = 30.0f;
    float mt_gain = 0.5f; // relative to the cell's own mean positive input
    float mt_decay = 0.7f;
    // Which levels have microtubules (with microtubule = 1): sum of 1 = line cells, 2 = sheet
    // cells, 4 = voxels. 7 = every cell. Each level's microtubules step on that level's clock.
    float mt_levels = 7.0f;
    // Microtubule controls (MicrotubuleLayer::Params::mode): 0 quantum, 1 expected value,
    // 2 classical stand-in, 3 shuffled outputs, 4 constant boost, 5 classical with chance,
    // 6 quantum timing but classical odds at the reduction (the control for mt_orch). mt_stream picks another random
    // stream for the reductions (the same brain with other chances).
    float mt_mode = 0.0f;
    float mt_stream = 0.0f;
    // Tubular inception (Orch-OR's orchestration through gap junctions): 1 = the voxels' tubes inside
    // each 4D cell are coupled (mt_orch_couple x the cell's mean <sx>) and reduce together with one
    // shared quantum draw when the 4D cell's summed superposition passes mt_orch_threshold, so one
    // reduction decides the whole 4D cell's pattern. Needs the 4D lattice (lattice4d). 0 = off.
    float mt_orch = 0.0f;
    float mt_orch_couple = 0.3f;
    float mt_orch_threshold = 300.0f;
    // Pattern separation at storage: while the matrix is encoding (modulator > 0), the input of
    // each voxel channel in the Memory, Reasoning and Output fields is scaled by
    // max(0, 1 - separation * M * load), where load is the share of that channel's learned-input
    // budget already in use (it already belongs to stored memories). A new memory then settles on
    // fresher cells wherever it differs from older, similar ones. Recall is unaffected (M = 0),
    // and the learned links from the cue lead back to the separated memory. 0 = off.
    float separation = 0.0f;
    // Learned cue route: the random feedforward sources a voxel hears from earlier fields
    // (link4d_spread) get a learned C3 x C3 block each, like the other learned links. While a
    // memory is written they learn which input pattern leads to the cells that store it; at
    // recall the cue follows that route to its memory. Transmitted like the learned 4D link
    // (muted while encoding with encoding_suppression_4d). The value scales how strongly the
    // learned route transmits (1 = like the other learned links). 0 = off (fixed sources only).
    float spread_plastic = 0.0f;
    // Recency weighting of a line's summary: the cell at line position k (the letter heard k ticks
    // ago) counts line_recency^k when the line reports to its sheet. The line itself keeps all its
    // activity; the weights are scaled to sum to the line length, so the total signal (and a held
    // letter's strength) is unchanged. In a stream the newest letters dominate, so a word is not
    // drowned by the words before it. 1 = all positions count the same.
    float line_recency = 1.0f;
    float downward_gain = 0.3f;
    // 1 = divisive fatigue in the 2D sheets (they relay the input; a weak but real input is
    // never silenced there): a tired cell fires more slowly instead of having its threshold
    // raised. The 3D cells (where memories live) always use the threshold rise.
    float fatigue_divisive2 = 0.0f;

    // 1D lines carry sequences intact, so homeostasis is off there by default (spec Section 3C).
    // theta_max is high enough that homeostasis can always catch up with a cell's drive;
    // a low ceiling lets strongly driven cells saturate and freeze.
    LevelParams level1{0.0f, 0.0f, 10.0f, 0.5f};
    // Slow homeostasis: fast threshold changes made the response to the same input drift.
    // Fatigue 0.6 (measured over the channels that fire): ends a memory that keeps itself
    // running after its input stops, without fighting recall while a cue is present.
    LevelParams level2{0.001f, 0.0f, 10.0f, 0.5f, 0.6f, 20.0f};
    LevelParams level3{0.001f, 0.0f, 10.0f, 0.5f, 0.6f, 20.0f};

    StartingRule rule;
    LearningParams learning;

    uint64_t seed = 0xC05305A1ull;

    uint64_t itemSeed() const { return codebook_seed ? uint64_t(codebook_seed) : seed; }
    size_t voxelsPerField() const { return size_t(field_dim) * field_dim * field_dim; }
    size_t voxels() const { return voxelsPerField() * kFields; }
    size_t sheetCellsPerVoxel() const { return size_t(sheet_dim) * sheet_dim; }
    size_t sheetCells() const { return voxels() * sheetCellsPerVoxel(); }
    size_t lineCells() const { return sheetCells() * line_len; }
    // Lines on one face of a field: the sensory and motor surfaces (spec Sections 6B, 6C).
    size_t surfaceLines() const { return size_t(field_dim) * field_dim * sheetCellsPerVoxel(); }
};

// "tiny" (fast smoke test), "dev" (test machine), "full" (spec size, needs ~18 GB at fp16).
Config makePreset(const std::string& name);

// Sets one tunable parameter from "name=value" (e.g. "upward_gain=1.0").
// These are the values rule evolution will search over (spec Section 5E).
void applySetting(Config& cfg, const std::string& assignment);

// The evolved rule: the best settings found by rule evolution, applied on top of a preset.
void applyEvolvedRule(Config& cfg);

// Names accepted by applySetting.
std::string settingNames();

} // namespace ncm
