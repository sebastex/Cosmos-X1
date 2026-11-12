#include "ncm/Somatic.hpp"

#include <algorithm>
#include <cmath>

#include "ncm/Random.hpp"

namespace ncm {

void SomaticNetwork::init(size_t cells, uint32_t channels, std::vector<uint32_t> nodeOf, uint32_t nodes, uint64_t seed, bool compare, bool local) {
    channels_ = channels;
    nodes_ = nodes;
    seed_ = seed;
    nodeOf_ = std::move(nodeOf);
    expect_.assign(cells * channels, 0.0f);
    a0_.assign(nodes, 1.0f); // every node starts definite in |0>
    a1r_.assign(nodes, 0.0f);
    a1i_.assign(nodes, 0.0f);
    held_.assign(nodes, 0.0f);
    evidence_.assign(nodes, 0.0f);
    cellDiff_.assign(cells, 0.0f);
    compare_ = compare;
    if (compare_) guess_.assign(cells * channels, 0.0f);
    local_ = local;
    if (local_) nodeSup_.assign(nodes, 0.0f), nodeSeen_.assign(nodes, 0);
}

void SomaticNetwork::onset() {
    if (!local_) return;
    for (uint32_t m = 0; m < nodes_; ++m) a0_[m] = 1.0f, a1r_[m] = 0.0f, a1i_[m] = 0.0f;
    std::fill(nodeSup_.begin(), nodeSup_.end(), 0.0f);
    std::fill(nodeSeen_.begin(), nodeSeen_.end(), uint64_t(0));
}

void SomaticNetwork::step(const float* state, const Params& p, bool hearing) {
    ++step_;
    const size_t cells = nodeOf_.size();
    const uint32_t C = channels_;
    const float k = 1.0f / std::max(1.0f, p.tau);
    if (compare_) { // compare with what was expected (against the background as it was before this step)
        std::vector<double> dot(nodes_, 0.0), nn(nodes_, 0.0), ng(nodes_, 0.0);
        const float kg = 1.0f / std::max(1.0f, p.compareTau);
        for (size_t v = 0; v < cells; ++v) { // in order, so the sums are the same every run
            const float* x = state + v * C;
            const float* e = expect_.data() + v * C;
            float* g = guess_.data() + v * C;
            const uint32_t m = nodeOf_[v];
            for (uint32_t c = 0; c < C; ++c) {
                const float d = x[c] - e[c];
                if (hearing) {
                    dot[m] += double(d) * g[c];
                    nn[m] += double(d) * d;
                    ng[m] += double(g[c]) * g[c];
                } else {
                    g[c] += kg * (d - g[c]); // listening: hold what it is doing now
                }
            }
        }
        if (hearing) {
            double fit = 0.0, w = 0.0;
            for (uint32_t m = 0; m < nodes_; ++m) {
                if (nn[m] <= 0.0 || ng[m] <= 0.0) continue;
                const double wm = std::sqrt(nn[m]);
                fit += wm * dot[m] / (wm * std::sqrt(ng[m]));
                w += wm;
            }
            mismatch_ = w > 0.0 ? float((1.0 - fit / w) / 2.0) : 0.0f;
        } else {
            mismatch_ = 0.0f;
        }
    }
    // Sense: each cell's firing above what it usually does (before the expectation learns this step).
    double sq = 0.0, ex = 0.0;
#pragma omp parallel for schedule(static) reduction(+ : sq, ex)
    for (int64_t vi = 0; vi < int64_t(cells); ++vi) {
        const size_t v = size_t(vi);
        const float* x = state + v * C;
        float* e = expect_.data() + v * C;
        float pos = 0.0f;
        for (uint32_t c = 0; c < C; ++c) {
            const float d = x[c] - e[c];
            sq += double(d) * d;
            pos += std::max(0.0f, d);
            e[c] += k * (x[c] - e[c]);
            ex += e[c];
        }
        cellDiff_[v] = pos;
    }
    spread_ = float(std::sqrt(sq / double(std::max<size_t>(1, cells * C)))) + 1e-6f;
    meanExpect_ = float(ex / double(std::max<size_t>(1, cells * C)));
    std::fill(evidence_.begin(), evidence_.end(), 0.0f);
    std::vector<uint32_t> count(nodes_, 0);
    for (size_t v = 0; v < cells; ++v) { // in order, so the sums are the same every run
        evidence_[nodeOf_[v]] += cellDiff_[v];
        ++count[nodeOf_[v]];
        // local: what this node senses, as a pattern (which of its cells are clearly different)
        if (local_ && cellDiff_[v] > spread_ * float(C)) nodeSeen_[nodeOf_[v]] = mix64(nodeSeen_[nodeOf_[v]] ^ (uint64_t(v) + 1));
    }
    for (uint32_t m = 0; m < nodes_; ++m)
        evidence_[m] = count[m] ? evidence_[m] / (float(count[m]) * float(C) * spread_) : 0.0f;

    if (p.mode == 2) { // classical: the difference is passed on directly, no state, no reduction
        for (uint32_t m = 0; m < nodes_; ++m) held_[m] = std::clamp(p.drive * evidence_[m], 0.0f, 1.0f);
        return;
    }
    // Quantum: every node turns by its own difference and the mean field of all nodes.
    double field = 0.0;
    for (uint32_t m = 0; m < nodes_; ++m) field += 2.0 * a0_[m] * a1r_[m]; // <sx> = 2 Re(conj(a0) a1)
    field /= double(std::max<uint32_t>(1, nodes_));
    const float eps = p.bias;
    double sup = 0.0;
    for (uint32_t m = 0; m < nodes_; ++m) {
        const float h = p.drive * evidence_[m] + p.couple * float(field);
        float r0 = a0_[m], i0 = 0.0f, r1 = a1r_[m], i1 = a1i_[m];
        const float om = std::sqrt(eps * eps + h * h);
        if (om > 1e-9f) { // a' = cos(om) a - i sin(om)/om H a: exact unitary
            const float cs = std::cos(om), sn = std::sin(om) / om;
            const float h0r = eps * r0 + h * r1, h0i = eps * i0 + h * i1;
            const float h1r = h * r0 - eps * r1, h1i = h * i0 - eps * i1;
            const float n0r = cs * r0 + sn * h0i, n0i = cs * i0 - sn * h0r;
            const float n1r = cs * r1 + sn * h1i, n1i = cs * i1 - sn * h1r;
            r0 = n0r, i0 = n0i, r1 = n1r, i1 = n1i;
        }
        const float m0 = std::sqrt(r0 * r0 + i0 * i0); // remove the global phase (a0 real >= 0)
        float c0 = m0, c1r, c1i;
        if (m0 > 1e-6f) c1r = (r1 * r0 + i1 * i0) / m0, c1i = (i1 * r0 - r1 * i0) / m0;
        else c0 = 0.0f, c1r = std::sqrt(r1 * r1 + i1 * i1), c1i = 0.0f;
        const float norm = std::sqrt(c0 * c0 + c1r * c1r + c1i * c1i);
        if (norm > 0.0f) c0 /= norm, c1r /= norm, c1i /= norm;
        a0_[m] = c0, a1r_[m] = c1r, a1i_[m] = c1i;
        const float p1 = c1r * c1r + c1i * c1i;
        sup += 4.0 * p1 * (1.0 - p1);
        held_[m] *= std::clamp(p.decay, 0.0f, 1.0f);
        if (local_) { // this node alone: reduce when its own build-up passes the threshold
            nodeSup_[m] += 4.0f * p1 * (1.0f - p1);
            if (nodeSup_[m] >= std::max(1e-3f, p.threshold)) {
                const float u = float(hashUniform(seed_ ^ mix64(p.stream + 11), kStreamMicrotubule, nodeSeen_[m] ^ mix64(uint64_t(m) + 0x51A)));
                const bool one = p.mode == 1 ? p1 > 0.5f : u < p1;
                a0_[m] = one ? 0.0f : 1.0f, a1r_[m] = one ? 1.0f : 0.0f, a1i_[m] = 0.0f;
                held_[m] = one ? 1.0f : 0.0f;
                nodeSup_[m] = 0.0f;
                ++reductions_;
            }
        }
    }
    if (local_) return;
    tau_ += float(sup / double(std::max<uint32_t>(1, nodes_)));
    if (tau_ < std::max(1e-3f, p.threshold)) return;
    // Reduction: all nodes at once, one shared draw.
    const float u = float(hashUniform(seed_ ^ mix64(p.stream + 11), kStreamMicrotubule, mix64(step_ * 0xD1B54A32D192ED03ull + 0x5EED)));
    for (uint32_t m = 0; m < nodes_; ++m) {
        const float p1 = a1r_[m] * a1r_[m] + a1i_[m] * a1i_[m];
        const bool one = p.mode == 1 ? p1 > 0.5f : u < p1;
        a0_[m] = one ? 0.0f : 1.0f, a1r_[m] = one ? 1.0f : 0.0f, a1i_[m] = 0.0f;
        held_[m] = one ? 1.0f : 0.0f;
    }
    tau_ = 0.0f;
    ++reductions_;
}

void SomaticNetwork::addPush(size_t cell, const float* state, float* in, const Params& p) const {
    const uint32_t C = channels_;
    float own = 0.0f;
    for (uint32_t c = 0; c < C; ++c) own += std::max(0.0f, in[c]);
    if (own <= 0.0f) return;
    own /= float(C);
    const float* x = state + cell * C;
    const float* e = expect_.data() + cell * C;
    const float held = held_[nodeOf_[cell]];
    const float gain = p.gain * (1.0f + p.compareGain * mismatch_); // what does not fit gets more attention
    for (uint32_t c = 0; c < C; ++c) {
        const float z = std::max(0.0f, x[c] - e[c]) / spread_;                // different from usual
        // 0..1: how much is background. Against the brain's mean expectation (usualRef 0) a channel that
        // fired once in a while already counts as background where most of the brain is silent.
        const float usual = e[c] / (e[c] + (p.usualRef > 0.0f ? p.usualRef : meanExpect_) + 1e-6f);
        in[c] += own * (gain * held * std::min(z, 3.0f) - p.quiet * usual);
    }
}

void SomaticNetwork::state(StateFile& st) {
    st.value(channels_);
    st.value(nodes_);
    st.value(step_);
    st.value(reductions_);
    st.value(spread_);
    st.value(meanExpect_);
    st.value(tau_);
    st.vec(expect_, "somatic expectations");
    st.vec(a0_, "somatic nodes");
    st.vec(a1r_, "somatic nodes");
    st.vec(a1i_, "somatic nodes");
    st.vec(held_, "somatic nodes");
    if (local_) { // only brains with local decisions
        st.vec(nodeSup_, "somatic local build-up");
        st.vec(nodeSeen_, "somatic local patterns");
    }
    if (compare_) { // only brains that compare (older brains load unchanged)
        st.vec(guess_, "somatic held expectations");
        st.value(mismatch_);
    }
}

} // namespace ncm
