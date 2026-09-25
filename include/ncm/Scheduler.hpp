#pragma once
#include <cstdint>

#include "ncm/Config.hpp"

namespace ncm {

// Level clocks (spec Section 4C). The 1D level ticks every step; each outer level
// runs 1/phi as fast as the level inside it. Time is accumulated rather than
// counted in whole sub-steps, because phi is irrational.
class LevelScheduler {
public:
    struct Tick {
        bool sheet; // run a 2D step after this 1D step
        bool voxel; // run a 3D step after this 1D step
    };

    Tick advance() {
        ++ticks1_;
        acc2_ += 1.0 / kPhi;
        acc3_ += 1.0 / (kPhi * kPhi);
        Tick t{acc2_ >= 1.0, acc3_ >= 1.0};
        if (t.sheet) {
            acc2_ -= 1.0;
            ++ticks2_;
        }
        if (t.voxel) {
            acc3_ -= 1.0;
            ++ticks3_;
        }
        return t;
    }

    uint64_t ticks1D() const { return ticks1_; }
    uint64_t ticks2D() const { return ticks2_; }
    uint64_t ticks3D() const { return ticks3_; }

private:
    double acc2_ = 0.0;
    double acc3_ = 0.0;
    uint64_t ticks1_ = 0, ticks2_ = 0, ticks3_ = 0;
};

} // namespace ncm
