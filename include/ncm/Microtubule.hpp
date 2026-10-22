#pragma once
// Microtubules (Orch-OR-like, local quantum mathematics), one inside every cell of a level.
//
// A microtubule is a cylinder of 13 protofilaments x len tubulin units. Each unit holds a
// superposition a0|0> + a1|1> (complex amplitudes; the unobservable global phase is removed, so
// a0 is real) and evolves by the exact unitary of H = bias sz + h sx, with
// h = couple x (sum of its 4 neighbours' <sx>, 3-start helical seam included) + drive x (activity
// of its cell's channel). The sx term is also the tunnelling term between |0> and |1>. The
// cylinder accumulates its superposition (sum of 4 |a0|^2 |a1|^2 per step); past the threshold it
// reduces objectively: every unit becomes 0 or 1 with the Born probabilities. The collapsed
// pattern shapes the cell's firing: each channel's input gains gain x (share of its units at 1)
// x (the cell's mean positive input), so it acts only on a cell that already has input (as in
// Orch-OR a reduction tips a neuron's firing, it does not create it); it fades by decay per step.
//
// Cost control (no loss of function): each unit is stored in 6 bytes (a0, re a1, im a1 as
// 16-bit fractions, renormalised every step); a cell's microtubule is created the first time the
// cell fires; only microtubules whose cell is active or that still hold a superposition are
// updated.

#include <cstdint>
#include <vector>

#include "ncm/State.hpp"

namespace ncm {

class MicrotubuleLayer {
public:
    struct Params {
        float couple = 0.3f, drive = 1.0f, bias = 0.2f, threshold = 30.0f, gain = 0.05f, decay = 0.7f;
        // Controls (to tell what the quantum part adds): 0 = quantum (Born-rule reduction),
        // 1 = expected value (reduction pushes by |a1|^2 and each unit takes its likelier state, no
        // chance), 2 = classical stand-in (no quantum state: the push is a fading trace of the cell's
        // recent firing per channel), 3 = shuffled (quantum, but each unit's collapse pushes a
        // different channel than the one that drives it), 4 = constant boost (gain x 0.5 on every
        // channel of a cell with input, no microtubule at all), 5 = classical with chance (no quantum
        // state; each unit fires at random with the probability of its channel's recent firing, and
        // the push is the share that fired: ordinary randomness without superposition or interference).
        int mode = 0;
        uint64_t stream = 0; // which random stream the reductions draw from (same brain, other chances)
    };

    bool on() const { return channels_ > 0; }
    // cells: cells in the level; channels: channels per cell; len: units along the cylinder.
    void init(size_t cells, uint32_t channels, uint32_t len, uint64_t seed, uint64_t salt);
    // One step of the level: `state` holds the level's new state (cells x channels), `active` the
    // cells with any activity.
    void step(const float* state, const std::vector<uint32_t>& active, const Params& p);
    // Adds the cell's held push (from its last objective reduction) to `in`.
    void addPush(size_t cell, float* in, float decay) const;
    uint64_t collapses() const { return collapses_; }
    size_t tubes() const { return owner_.size(); }
    void state(StateFile& st);

private:
    static constexpr uint32_t kNone = UINT32_MAX;
    uint32_t channels_ = 0, len_ = 0, units_ = 0;
    uint64_t seed_ = 0, salt_ = 0, step_ = 0, collapses_ = 0;
    int mode_ = 0;
    float gain_ = 0.0f;
    std::vector<uint32_t> index_;  // per cell: its microtubule, or kNone
    std::vector<uint32_t> owner_;  // per microtubule: its cell
    std::vector<int16_t> amp_;     // per microtubule: units x (a0, re a1, im a1), scaled by 32767
    std::vector<float> tau_;       // accumulated superposition
    std::vector<float> out_;       // push on the cell's channels at the time of `stamp_`
    std::vector<uint64_t> stamp_;  // step at which out_ was last brought up to date
    std::vector<uint8_t> quiet_;   // 1 = definite (no superposition left)
    std::vector<uint64_t> visit_;  // step at which the microtubule was last updated
    std::vector<uint32_t> live_;   // microtubules still holding a superposition
    std::vector<uint32_t> chan_;   // per unit: the channel of the cell that drives it
    std::vector<uint32_t> outChan_; // per unit: the channel its collapse pushes in the shuffled control
};

} // namespace ncm
