#pragma once
// Activity monitor: a passive recorder, separate from the engine. After each tick it reads the
// state of every level (1D line cells, 2D sheet cells, 3D voxels) and of every 4D cell (the
// 4D-lattice drive and which clocks stepped) and writes it to a file. It only reads: the brain
// runs exactly the same with or without it.
//
// Turned on by the environment, for any test:
//   NCM_MONITOR=<folder>     one file per brain session: <folder>/session<k>.ncmmon
//   NCM_MONITOR_EVERY=<n>    record every n-th tick (default 1 = every tick)
//   NCM_MONITOR_FROM=<t>, NCM_MONITOR_TO=<t>   record only ticks in this range
//   NCM_MONITOR_VOXELS=1     also record every active voxel with its 16 channels
// Read with tools/monitor/read_monitor.py.
//
// Roots of an idea: an active 4D cell is the stem; the active voxels inside it, the active sheet
// cells inside those voxels and the active line cells inside those sheet cells are its roots,
// one level deeper each. Recorded per tick: stems, and the active cells that hang from an active
// cell one level up (or hang from a silent one: cut roots).
//
// Groups: the voxels are grouped into 4D cells (4 x 4 x 4 blocks of the voxel space in each of
// the 4 w-slices, i.e. Pharmakon's 4D cells; in Prime the same blocks of each field), so both
// versions are recorded the same way.

#include <cstdint>
#include <cstdio>
#include <memory>
#include <string>
#include <vector>

#include "ncm/Config.hpp"

namespace ncm {

class NeuralCellularMatrix;

class Monitor {
public:
    struct Event {
        uint64_t tick = 0;     // 1D tick of the session
        bool sheet = false;    // a 2D step ran this tick
        bool voxel = false;    // a 3D step ran this tick
        bool fourD = false;    // a 4D step ran this tick
        bool sensory = false;  // input was heard this tick
        bool learning = false; // learning was allowed this tick
        char heard = 0;        // the character heard (0 = silence)
        float modulator = 0.0f;
    };

    // A monitor if NCM_MONITOR is set, otherwise nullptr.
    static std::unique_ptr<Monitor> fromEnv(const Config& cfg);

    Monitor(const Config& cfg, const std::string& file, uint64_t every, uint64_t from, uint64_t to, bool voxels);
    ~Monitor();
    Monitor(const Monitor&) = delete;
    Monitor& operator=(const Monitor&) = delete;

    void record(const NeuralCellularMatrix& m, const Event& e);

private:
    std::FILE* f_ = nullptr;
    uint64_t every_ = 1, from_ = 0, to_ = UINT64_MAX;
    bool voxels_ = false;
    uint32_t N_ = 0, SS_ = 0, L_ = 0, A_ = 1, B_ = 1, G_ = 1;
    size_t V_ = 0, Vf_ = 0;
    float level1_ = 0.0f, level2_ = 0.0f, level3_ = 0.0f;
    std::vector<uint32_t> groupOf_; // 4D cell of each voxel
    // Per voxel scratch: active cells and summed activity at each level.
    std::vector<uint32_t> a1_, a2_, a3_;
    // Roots of an idea (per voxel): active sheet cells inside an active voxel / inside a silent
    // one, and active line cells inside an active sheet cell of an active voxel / inside a silent
    // sheet cell.
    std::vector<uint32_t> r2_, o2_, r1_, o1_;
    std::vector<float> s1_, s2_, s3_, d4_;
};

} // namespace ncm
