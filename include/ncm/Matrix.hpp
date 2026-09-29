#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

#include "ncm/Buffer.hpp"
#include "ncm/Config.hpp"

namespace ncm {

struct LevelStats {
    std::array<double, kFields> active_fraction{}; // share of cells counted as active
    std::array<double, kFields> mean{};            // mean channel value
};

// Running totals of learning activity, for diagnostics.
struct LearningStats {
    uint64_t calls = 0;    // learn() calls that applied a non-zero rate
    uint64_t learners = 0; // voxel updates (active voxels per call, summed)
    double change = 0.0;   // total absolute Hebbian change to learned strengths
    double scaled = 0.0;   // total strength removed by synaptic scaling
};

struct MatrixStats {
    LevelStats line;  // 1D
    LevelStats sheet; // 2D
    LevelStats voxel; // 3D
};

// The Neural Cellular Matrix: four 3D fields; every voxel holds a 2D sheet;
// every sheet cell holds a 1D line (spec Section 2).
//
// Each level advances with its own step function, written as "update every cell
// from the current buffers into the next buffers" with no hidden state, so the
// same logic ports directly to GPU compute shaders later (spec Stage 6).
// Diagnostic: where a field's 3D input came from on the last step. `plastic` is the learned part,
// `total` all positive input (learned + fixed paths); the *Active sums cover firing voxels only.
struct DriveBreakdown {
    double plastic = 0.0, total = 0.0, plasticActive = 0.0, totalActive = 0.0, activeVoxels = 0.0;
};
// Diagnostic: how the learned connections of a field are used. Fill = a channel's incoming learned
// strength / plastic budget; topSourceShare = share of all outgoing learned strength held by the
// top 1% of source channels (hubs).
struct WeightHealth {
    double meanFill = 0.0, shareFull = 0.0, shareUsed = 0.0, topSourceShare = 0.0;
};

class NeuralCellularMatrix {
public:
    explicit NeuralCellularMatrix(const Config& cfg);

    NeuralCellularMatrix(const NeuralCellularMatrix&) = delete;
    NeuralCellularMatrix& operator=(const NeuralCellularMatrix&) = delete;

    void step1D();
    void step2D();
    void step3D();

    // Hebbian learning (spec Section 5A), applied right after step3D():
    // association + order - Oja normalization, scaled by the modulator M in [0, 1].
    // Learns the 3D neighbourhood, long-range and 4D connections from excitatory
    // sources, and each voxel's sheet modulation. M = 0 leaves everything frozen.
    void learn(float modulator);

    // The current surprise modulator M in [0, 1]. Besides gating learning it sets the
    // mode: high M = encoding (learned connections turned down), low M = recall.
    void setModulator(float modulator) { modulator_ = modulator; }

    // Share of a fingerprint's motor-surface lines whose exit cells are active:
    // how strongly the matrix is "about to say" that character (spec Section 5A).
    double motorOverlap(const std::vector<uint32_t>& fingerprint) const;

    // Sensory surface (spec Section 6B): face x = 0 of the Input field.
    // `active_lines` are surface line indices whose entry cells fire this tick;
    // all other entry cells are held silent while input is on.
    void setSensoryInput(const std::vector<uint32_t>& active_lines);
    void clearSensoryInput();

    // Motor surface (spec Section 6C): face x = N-1 of the Output field.
    // Clamping places a pattern on the exit cells (imitation, spec Section 7B).
    void clampMotor(const std::vector<uint32_t>& active_lines);
    void releaseMotor();
    // Surface line indices whose exit cells are currently active.
    std::vector<uint32_t> readMotorExit() const;

    // Silences every cell (states only; learned connections and thresholds are kept).
    // A diagnostic, used to test whether lingering activity causes memories to merge.
    void clearActivity();

    MatrixStats computeStats() const;
    const LearningStats& learningStats() const { return learnStats_; }
    const std::array<float, kFields>& fieldGains() const { return fieldGain_; }
    // Total strength of the plastic (learned) part of all connections.
    double totalPlasticStrength() const;
    // Diagnostic: total learned (plastic) drive the pattern `from` would send into the pattern
    // `to` (both 3D state vectors, e.g. accumulated activity), normalized by both patterns'
    // norms. Measures a stored association directly, independent of recall dynamics.
    double plasticFlow(const std::vector<double>& from, const std::vector<double>& to);
    std::array<DriveBreakdown, kFields> driveBreakdown() const;
    std::array<WeightHealth, kFields> weightHealth();

    const Config& config() const { return cfg_; }
    const AVec<float>& lineState() const { return s1_.cur; }
    const AVec<float>& sheetState() const { return s2_.cur; }
    const AVec<float>& voxelState() const { return s3_.cur; }
    size_t memoryBytes() const;

    size_t voxelIndex(uint32_t field, uint32_t x, uint32_t y, uint32_t z) const {
        return ((size_t(field) * N_ + z) * N_ + y) * N_ + x;
    }

private:
    void initInhibitory();
    void initSharedRules();
    void initPositionWeights();
    void initVoxelWeights();
    void initSurfaces();
    void applySurfaceClamps();

    // Calls fn(block) for every learned C3 x C3 block entering voxel v: neighbourhood
    // (except self-persistence), long-range and 4D, from excitatory sources only.
    template <class Fn>
    void forEachLearnedBlock(size_t v, Fn&& fn);

    Config cfg_;
    uint32_t N_, S_, L_;
    size_t V_, Vf_, SS_, Q_, P_;

    LevelState s1_; // 1D line cells, C1 channels
    LevelState s2_; // 2D sheet cells, C2 channels
    LevelState s3_; // 3D voxels, C3 channels

    std::vector<uint8_t> inhib2_; // inhibitory flag per sheet cell
    std::vector<uint8_t> inhib3_; // inhibitory flag per voxel

    // Drive per cell for the local competition (spec Section 3C); scratch, rewritten every step.
    AVec<float> drive2_;
    AVec<float> drive3_;
    AVec<float> diagPlastic3_; // learned part of each voxel's input on the last step (diagnostic)
    AVec<float> diagInput3_;   // all positive input of each voxel on the last step (diagnostic)
    AVec<float> resource3_;    // short-term depression: transmitter resource per voxel channel

    // Fatigue per cell (2D and 3D), and each voxel's long-run average activity (covariance learning).
    AVec<float> fatigue2_;
    AVec<float> fatigue3_;
    AVec<float> average3_;
    // Per line: 1 if the line is entirely silent in s1_.cur / s1_.next (event-driven skipping).
    std::vector<uint8_t> lineQuietCur_, lineQuietNext_;
    std::vector<float> preRoom_; // per source voxel channel: unused share of its outgoing budget
    // Per voxel: 1 if its whole sheet is silent in s2_.cur / s2_.next; sheetSkip_ marks the
    // sheets skipped in the current 2D step.
    std::vector<uint8_t> sheetQuietCur_, sheetQuietNext_, sheetSkip_;
    AVec<float> orderTrace3_; // per voxel channel: decaying recent activity (order timing window)
    AVec<float> trace3_;    // per voxel channel: short running average of activity (trace-based association)
    AVec<float> averageN3_; // long-run average of the normalized firing pattern (normalized plasticity)

    LearningStats learnStats_;
    float modulator_ = 0.0f;

    // Per-field gain on incoming signals (gain control), adapted toward the target activity.
    std::array<float, kFields> fieldGain_{1.0f, 1.0f, 1.0f, 1.0f};
    std::vector<float> voxelGain_; // local gain control: per-voxel gain on incoming signals
    AVec<float> P2_;               // sheet learning: per sheet cell, C2 x C2 block from its summed neighbourhood

    // Shared rules per level (evolved in Stage 5): row-major [out][in] channel matrices.
    AVec<float> W1_; // 3 offsets (left, self, right) x C1 x C1
    AVec<float> W2_; // 9 offsets (3 x 3) x C2 x C2

    // Fixed random position weights (spec Section 2B), shared per level.
    AVec<float> U1_; // line position k -> sheet cell:  L  x C2 x C1
    AVec<float> D1_; // sheet cell -> line entry cell:  C1 x C2 (the carry spreads it over positions)
    AVec<float> U2_; // sheet position  -> voxel:       SS x C3 x C2
    AVec<float> D2_; // voxel -> sheet position:        SS x C2 x C3

    // Learned connections (spec Section 2C) = fixed scaffold (StartingRule strengths, a
    // scaled one-to-one channel map applied on the fly) + the plastic parts stored here.
    AVec<float> W3_;               // per voxel: 27 neighbourhood offsets x C3 x C3 (self block unused)
    std::vector<uint32_t> lrTarget_; // per voxel: long-range target voxels (fixed at random)
    std::vector<uint32_t> spreadPos_; // per voxel: random positions (index within a field) feeding it from earlier fields
    uint32_t spreadK_ = 0;
    std::vector<uint32_t> inputDepthPos_; // per Input-field voxel: random sensory-face positions (y + z*N)            // random 4D sources per voxel (link4d_spread, scaled with size if set)
    AVec<float> WL_;               // per voxel: long-range links x C3 x C3
    AVec<float> H_;                // per voxel: 3 other fields x C3 x C3 (4D link)
    // Consolidated (slow) plastic parts, same layout as W3_, WL_ and H_; empty when
    // consolidation is off. Transmission uses fast + slow.
    AVec<float> S3_, SL_, SH_;
    float* slowOf(float* fastBlock) {
        auto map = [&](AVec<float>& fast, AVec<float>& slow) -> float* {
            if (slow.empty() || fastBlock < fast.data() || fastBlock >= fast.data() + fast.size()) return nullptr;
            return slow.data() + (fastBlock - fast.data());
        };
        if (float* s = map(W3_, S3_)) return s;
        if (float* s = map(WL_, SL_)) return s;
        return map(H_, SH_);
    }
    AVec<float> M2_;               // per voxel: C2 x C2 sheet modulation

    // Surfaces: surface line index -> sheet cell index q.
    std::vector<uint32_t> sensoryQ_;
    std::vector<uint32_t> motorQ_;
    std::vector<uint8_t> sensoryDrive_;
    std::vector<uint8_t> motorDrive_;
    bool sensoryOn_ = false;
    bool motorOn_ = false;
};

} // namespace ncm
