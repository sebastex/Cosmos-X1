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
class NeuralCellularMatrix {
public:
    explicit NeuralCellularMatrix(const Config& cfg);

    NeuralCellularMatrix(const NeuralCellularMatrix&) = delete;
    NeuralCellularMatrix& operator=(const NeuralCellularMatrix&) = delete;

    void step1D();
    void step2D();
    void step3D();

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

    MatrixStats computeStats() const;

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

    // Shared rules per level (evolved in Stage 5): row-major [out][in] channel matrices.
    AVec<float> W1_; // 3 offsets (left, self, right) x C1 x C1
    AVec<float> W2_; // 9 offsets (3 x 3) x C2 x C2

    // Fixed random position weights (spec Section 2B), shared per level.
    AVec<float> U1_; // line position k -> sheet cell:  L  x C2 x C1
    AVec<float> D1_; // sheet cell -> line entry cell:  C1 x C2 (the carry spreads it over positions)
    AVec<float> U2_; // sheet position  -> voxel:       SS x C3 x C2
    AVec<float> D2_; // voxel -> sheet position:        SS x C2 x C3

    // Learned connections (spec Section 2C). Learning itself arrives in Stage 1.
    AVec<float> W3_;               // per voxel: 27 neighbourhood offsets x C3 x C3
    std::vector<uint32_t> lrTarget_; // per voxel: long-range target voxels (fixed at random)
    AVec<float> WL_;               // per voxel: long-range links x C3 x C3
    AVec<float> H_;                // per voxel: 3 other fields x C3 x C3 (4D link)
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
