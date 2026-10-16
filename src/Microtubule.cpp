#include "ncm/Microtubule.hpp"

#include <algorithm>
#include <cmath>

#include "ncm/Random.hpp"

namespace ncm {

namespace {

constexpr float kScale = 32767.0f;

inline int16_t q16(float x) { return int16_t(std::lround(std::clamp(x, -1.0f, 1.0f) * kScale)); }

} // namespace

void MicrotubuleLayer::init(size_t cells, uint32_t channels, uint32_t len, uint64_t seed, uint64_t salt) {
    channels_ = channels;
    len_ = std::clamp<uint32_t>(len, 4, 64);
    units_ = 13 * len_;
    seed_ = seed;
    salt_ = salt;
    index_.assign(cells, kNone);
    chan_.resize(units_);
    for (uint32_t u = 0; u < units_; ++u)
        chan_[u] = std::min<uint32_t>(channels - 1, uint32_t(hashUniform(seed, kStreamMicrotubule, salt * 4096 + u) * channels));
}

void MicrotubuleLayer::addPush(size_t cell, float* in, float decay) const {
    const uint32_t id = index_[cell];
    if (id == kNone) return;
    const float f = std::pow(std::clamp(decay, 0.0f, 1.0f), float(step_ - stamp_[id]));
    if (f < 1e-4f) return;
    // The reduction shapes how a cell that already has input fires; it never creates activity:
    // the push is relative to the cell's own mean positive drive.
    float own = 0.0f;
    for (uint32_t c = 0; c < channels_; ++c) own += std::max(0.0f, in[c]);
    if (own <= 0.0f) return;
    own /= float(channels_);
    const float* o = out_.data() + size_t(id) * channels_;
    for (uint32_t c = 0; c < channels_; ++c) in[c] += f * o[c] * own;
}

void MicrotubuleLayer::step(const float* state, const std::vector<uint32_t>& active, const Params& p) {
    const uint64_t now = ++step_;
    const uint32_t C = channels_, T = units_, P = 13, Lm = len_;
    // A cell's microtubule is made the first time the cell fires (every unit definite in |0>).
    for (uint32_t cell : active) {
        if (index_[cell] != kNone) continue;
        index_[cell] = uint32_t(owner_.size());
        owner_.push_back(cell);
        for (uint32_t u = 0; u < T; ++u) {
            amp_.push_back(int16_t(kScale));
            amp_.push_back(0);
            amp_.push_back(0);
        }
        tau_.push_back(0.0f);
        out_.insert(out_.end(), C, 0.0f);
        stamp_.push_back(now);
        quiet_.push_back(1);
        visit_.push_back(0);
    }
    // Update: microtubules of active cells, and those still holding a superposition.
    std::vector<uint32_t> list;
    list.reserve(active.size() + live_.size());
    for (uint32_t cell : active) {
        const uint32_t id = index_[cell];
        if (visit_[id] != now) visit_[id] = now, list.push_back(id);
    }
    for (uint32_t id : live_)
        if (visit_[id] != now) visit_[id] = now, list.push_back(id);

    const float J = p.couple, D = p.drive, eps = p.bias, thr = std::max(1e-3f, p.threshold);
    const float decay = std::clamp(p.decay, 0.0f, 1.0f);
    const int64_t n = int64_t(list.size());
    uint64_t collapses = 0;
#pragma omp parallel for schedule(dynamic, 64) reduction(+ : collapses)
    for (int64_t i = 0; i < n; ++i) {
        const uint32_t id = list[size_t(i)];
        const float* x = state + size_t(owner_[id]) * C;
        int16_t* a = amp_.data() + size_t(id) * T * 3;
        float a0[13 * 64], br[13 * 64], bi[13 * 64], sx[13 * 64];
        for (uint32_t u = 0; u < T; ++u) {
            a0[u] = a[u * 3] / kScale, br[u] = a[u * 3 + 1] / kScale, bi[u] = a[u * 3 + 2] / kScale;
            sx[u] = 2.0f * a0[u] * br[u]; // <sx> = 2 Re(conj(a0) a1), a0 real
        }
        float sup = 0.0f;
        for (uint32_t u = 0; u < T; ++u) {
            const uint32_t pf = u % P, k = u / P;
            float field = 0.0f; // neighbours on the cylinder
            if (k > 0) field += sx[u - P];
            if (k + 1 < Lm) field += sx[u + P];
            if (pf + 1 < P) field += sx[u + 1];
            else if (k + 3 < Lm) field += sx[(k + 3) * P];
            if (pf > 0) field += sx[u - 1];
            else if (k >= 3) field += sx[(k - 3) * P + P - 1];
            const float h = J * field + D * x[chan_[u]];
            float r0 = a0[u], i0 = 0.0f, r1 = br[u], i1 = bi[u];
            const float om = std::sqrt(eps * eps + h * h);
            if (om > 1e-9f) {
                // a' = cos(om) a - i sin(om)/om H a, H = [[eps, h], [h, -eps]]: exact unitary.
                const float cs = std::cos(om), sn = std::sin(om) / om;
                const float h0r = eps * r0 + h * r1, h0i = eps * i0 + h * i1;
                const float h1r = h * r0 - eps * r1, h1i = h * i0 - eps * i1;
                const float n0r = cs * r0 + sn * h0i, n0i = cs * i0 - sn * h0r;
                const float n1r = cs * r1 + sn * h1i, n1i = cs * i1 - sn * h1r;
                r0 = n0r, i0 = n0i, r1 = n1r, i1 = n1i;
            }
            // Remove the global phase (a0 real >= 0) and renormalise.
            const float m0 = std::sqrt(r0 * r0 + i0 * i0);
            float c0 = m0, c1r, c1i;
            if (m0 > 1e-6f) {
                c1r = (r1 * r0 + i1 * i0) / m0, c1i = (i1 * r0 - r1 * i0) / m0;
            } else {
                c0 = 0.0f, c1r = std::sqrt(r1 * r1 + i1 * i1), c1i = 0.0f;
            }
            const float norm = std::sqrt(c0 * c0 + c1r * c1r + c1i * c1i);
            if (norm > 0.0f) c0 /= norm, c1r /= norm, c1i /= norm;
            a[u * 3] = q16(c0), a[u * 3 + 1] = q16(c1r), a[u * 3 + 2] = q16(c1i);
            const float p1 = c1r * c1r + c1i * c1i;
            sup += 4.0f * p1 * (1.0f - p1);
        }
        tau_[id] += sup;
        quiet_[id] = sup < 1e-5f ? 1 : 0;
        if (tau_[id] < thr) continue;
        // Objective reduction: every unit becomes definite, 1 with probability |a1|^2 (Born rule).
        float ones[64] = {}, count[64] = {};
        for (uint32_t u = 0; u < T; ++u) {
            const float r1 = a[u * 3 + 1] / kScale, i1 = a[u * 3 + 2] / kScale;
            const bool one = hashUniform(seed_ ^ (salt_ * 0x9E3779B97F4A7C15ull), kStreamMicrotubule,
                                         mix64(now * 0x9E3779B97F4A7C15ull + 0x632BE59BD9B4E019ull) ^ (uint64_t(owner_[id]) * T + u)) < r1 * r1 + i1 * i1; // step and unit hashed apart (no overlap for 16M cells)
            a[u * 3] = one ? 0 : int16_t(kScale), a[u * 3 + 1] = one ? int16_t(kScale) : 0, a[u * 3 + 2] = 0;
            count[chan_[u]] += 1.0f;
            ones[chan_[u]] += one ? 1.0f : 0.0f;
        }
        float* o = out_.data() + size_t(id) * C;
        const float f = std::pow(decay, float(now - stamp_[id]));
        for (uint32_t c = 0; c < C; ++c) o[c] = f * o[c] + (count[c] > 0.0f ? p.gain * ones[c] / count[c] : 0.0f);
        stamp_[id] = now;
        tau_[id] = 0.0f;
        quiet_[id] = 1;
        ++collapses;
    }
    collapses_ += collapses;
    live_.clear();
    for (uint32_t id : list)
        if (!quiet_[id]) live_.push_back(id);
}

void MicrotubuleLayer::state(StateFile& st) {
    st.value(channels_);
    st.value(len_);
    st.value(units_);
    st.value(step_);
    st.value(collapses_);
    st.vec(index_, "microtubules");
    st.vec(owner_, "microtubules");
    st.vec(amp_, "microtubules");
    st.vec(tau_, "microtubules");
    st.vec(out_, "microtubules");
    st.vec(stamp_, "microtubules");
    st.vec(quiet_, "microtubules");
    st.vec(visit_, "microtubules");
    st.vec(live_, "microtubules");
    st.vec(chan_, "microtubules");
}

} // namespace ncm
