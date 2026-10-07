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
    // Word competition (version G, after lexical competition in spoken-word recognition and
    // lateral inhibition in the naming game): in recall mode the voxel competition is stricter,
    // winners3_recall winners within radius inhibition_radius3_recall, so the strongest recalled
    // memory wins clean and partly matching ones are pushed out. 0 = same as while encoding.
    uint32_t winners3_recall = 0;
    uint32_t inhibition_radius3_recall = 0;
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
