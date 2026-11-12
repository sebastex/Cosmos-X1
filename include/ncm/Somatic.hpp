#pragma once
#include <cstdint>
#include <vector>

#include "ncm/State.hpp"

namespace ncm {

// The somatic network: one network wired to every voxel the same way. It remembers what each cell
// usually does (a slow expectation per cell channel, so words heard all the time become background),
// senses what is different now (firing above that expectation, measured against the whole brain's
// spread of differences), and tells every cell back: the part of its firing that is different gets
// a push, the part that is only the usual background is turned down.
//
// Which differences matter is decided with the microtubules' quantum rule: every node (one per 4D
// cell) is a two-level system driven by its cells' difference and by the mean field of all nodes
// (every node feels every other), evolving by the exact unitary a' = exp(-iH) a, H = [[eps, h],
// [h, -eps]]. Superposition builds up, and past a threshold all nodes reduce together with one
// shared draw (Born rule, |a1|^2): the network settles on one definite set of differences at a time.
// Controls: mode 1 = expected value (no chance), mode 2 = classical (the difference passed on directly).
class SomaticNetwork {
public:
    struct Params {
        float gain = 1.0f;       // push to the different part of a cell's firing
        float quiet = 0.5f;      // turning down the usual part
        float tau = 2000.0f;     // 3D steps the expectation remembers
        float drive = 2.0f;      // how strongly a node's difference turns its state
        float couple = 0.3f;     // how strongly every node feels the others (mean field)
        float bias = 0.2f;       // eps: each node's own energy split
        float threshold = 3.0f;  // superposition (summed over nodes, per node) before all reduce
        float decay = 0.9f;      // per 3D step, how long a settled decision lasts
        int mode = 0;
        uint64_t stream = 0;
        float compare = 0.0f;     // > 0: compare what is heard with what was expected (see mismatch())
        float compareTau = 2.0f;  // 3D steps: how fast the held expectation follows the brain while it listens
        float compareGain = 0.0f; // how much "does not fit" raises the push to what is different
        float usualRef = 0.0f;
        // local (soma_local): each node builds up its own superposition from the start of each word and
        // reduces alone when it passes the threshold; its draw is seeded by the pattern it senses (the
        // same word gives the same outcome, different words different ones, still by the Born rule).
        bool local = false;    // > 0: a channel counts as background against this fixed level (not the brain's mean)
    };
    void init(size_t cells, uint32_t channels, std::vector<uint32_t> nodeOf, uint32_t nodes, uint64_t seed, bool compare = false, bool local = false);
    bool on() const { return !nodeOf_.empty(); }
    // After every 3D step: update expectations, sense the differences, evolve and maybe reduce.
    void step(const float* state, const Params& p, bool hearing = false);
    // A word begins (local mode): every node starts a fresh build-up from |0> (decisions already held fade as before).
    void onset();
    // Comparing (compare): while nothing is heard every cell holds what it is now doing beyond its
    // usual background - what it expects. While something is heard each node measures how well its
    // cells' firing (beyond the background) matches what they held; over the whole brain (weighted by
    // how much each node fires) that is one fit in [-1, 1]. mismatch() = (1 - fit) / 2, 0..1.
    float mismatch() const { return mismatch_; }
    // Into a cell's input (relative to its own mean positive drive, so it shapes, never creates).
    void addPush(size_t cell, const float* state, float* in, const Params& p) const;
    void state(StateFile& st);
    uint64_t reductions() const { return reductions_; }

private:
    uint32_t channels_ = 0, nodes_ = 0;
    uint64_t seed_ = 0, step_ = 0, reductions_ = 0;
    std::vector<uint32_t> nodeOf_;
    std::vector<float> expect_;           // per cell channel: what it usually does
    std::vector<float> a0_, a1r_, a1i_;   // per node: amplitudes (a0 real)
    std::vector<float> held_;             // per node: the settled decision (1 = its differences matter)
    std::vector<float> evidence_;         // per node: its cells' difference, last step
    float spread_ = 1.0f, meanExpect_ = 0.0f, tau_ = 0.0f;
    std::vector<float> cellDiff_;         // scratch: per cell, summed positive difference
    std::vector<float> nodeSup_;          // local: per node, superposition built up since the word began
    std::vector<uint64_t> nodeSeen_;      // local: per node, a hash of the cells it senses as different (this word)
    bool local_ = false;
    bool compare_ = false;
    std::vector<float> guess_;            // per cell channel: what it held while listening (compare)
    float mismatch_ = 0.0f;
};

} // namespace ncm
