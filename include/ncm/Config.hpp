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
    float active_level;     // mean channel value at which a cell counts as active (statistics, readout)
};

// Hand-set starting rule. Stage 5 (rule evolution) replaces these values.
// Values found by the Stage 0 parameter search: activity spreads through all
// four fields, stays sparse, lingers after input and then fades.
struct StartingRule {
    float line_carry      = 1.0f;  // 1D: weight from the left neighbour, carries sequences toward the exit
    float sheet_self      = 0.4f;  // 2D: persistence of a cell's own state
    float sheet_neighbour = 0.06f; // 2D: each of the 8 lateral neighbours
    float voxel_self      = 0.5f;  // 3D: persistence of a voxel's own state
    float voxel_neighbour = 0.02f; // 3D: each of the 26 neighbours
    float voxel_noise     = 0.01f; // 3D: random spread of the initial per-voxel weights
    float long_range      = 0.05f; // 3D: long-range link strength
    float link4d          = 0.2f;  // 4D: link strength to each other field
};

struct Config {
    // Geometry (spec Section 2A). The spec's full size is 32 / 16 / 16.
    uint32_t field_dim        = 16;
    uint32_t sheet_dim        = 8;
    uint32_t line_len         = 16;
    uint32_t long_range_links = 4;

    float target_activity  = 0.02f; // spec Section 3B
    float inhibitory_share = 0.20f; // spec Section 3C, applies to 2D and 3D cells
    // Inhibitory connections are stronger than excitatory ones so that 20% of cells can
    // balance the other 80% (balanced excitation and inhibition, as in cortex).
    float inhibitory_strength = 4.0f;

    // Position weights are scaled by 1/sqrt(fan-in), so an upward gain of 1 keeps a
    // summary's strength roughly equal to what it summarizes (spec Section 2B).
    float upward_gain   = 1.0f;
    float downward_gain = 0.3f;

    // 1D lines carry sequences intact, so homeostasis is off there by default (spec Section 3C).
    // theta_max is high enough that homeostasis can always catch up with a cell's drive;
    // a low ceiling lets strongly driven cells saturate and freeze.
    LevelParams level1{0.0f, 0.0f, 10.0f, 0.5f};
    LevelParams level2{0.01f, 0.0f, 10.0f, 0.5f};
    LevelParams level3{0.01f, 0.0f, 10.0f, 0.5f};

    StartingRule rule;

    uint64_t seed = 0xC05305A1ull;

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

// Names accepted by applySetting.
std::string settingNames();

} // namespace ncm
