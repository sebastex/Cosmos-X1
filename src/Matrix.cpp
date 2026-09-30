#include "ncm/Matrix.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>

#include "ncm/Random.hpp"

namespace ncm {
namespace {

// y += scale * (A x), with A row-major [R][Cc]. Sizes are compile-time constants so the
// compiler can unroll fully and vectorize across rows; each row's sum keeps the same order,
// so results are bit-identical to the generic loop.
template <uint32_t R, uint32_t Cc>
inline void matvecFixed(const float* __restrict A, const float* __restrict x, float* __restrict y, float scale) {
    float acc[R];
    for (uint32_t r = 0; r < R; ++r) acc[r] = 0.0f;
    for (uint32_t c = 0; c < Cc; ++c) {
        const float xc = x[c];
        for (uint32_t r = 0; r < R; ++r) acc[r] += A[size_t(r) * Cc + c] * xc;
    }
    for (uint32_t r = 0; r < R; ++r) y[r] += scale * acc[r];
}

// Dispatches the block sizes the matrix uses to the fixed-size kernel.
inline void matvecAdd(const float* __restrict A, const float* __restrict x, float* __restrict y,
                      uint32_t rows, uint32_t cols, float scale) {
    if (rows == C1 && cols == C1) return matvecFixed<C1, C1>(A, x, y, scale);
    if (rows == C1 && cols == C2) return matvecFixed<C1, C2>(A, x, y, scale);
    if (rows == C2 && cols == C1) return matvecFixed<C2, C1>(A, x, y, scale);
    if (rows == C2 && cols == C2) return matvecFixed<C2, C2>(A, x, y, scale);
    if (rows == C2 && cols == C3) return matvecFixed<C2, C3>(A, x, y, scale);
    if (rows == C3 && cols == C2) return matvecFixed<C3, C2>(A, x, y, scale);
    if (rows == C3 && cols == C3) return matvecFixed<C3, C3>(A, x, y, scale);
    for (uint32_t r = 0; r < rows; ++r) {
        const float* a = A + size_t(r) * cols;
        float acc = 0.0f;
        for (uint32_t c = 0; c < cols; ++c) acc += a[c] * x[c];
        y[r] += scale * acc;
    }
}

inline void setIdentity(float* A, uint32_t n, float value) {
    for (uint32_t i = 0; i < n; ++i) A[size_t(i) * n + i] = value;
}

// y += scale * x: a scaffold connection, a scaled one-to-one channel map.
inline void addScaled(const float* __restrict x, float* __restrict y, uint32_t n, float scale) {
    for (uint32_t c = 0; c < n; ++c) y[c] += scale * x[c];
}

// Bounded, sparse activation f (spec Section 4A): rectified and capped at 1.
inline float activate(float x) { return std::clamp(x, 0.0f, 1.0f); }

// Homeostasis (spec Section 3C) tracks a cell's mean activity, so over time each
// cell averages the target level: strongly on a small share of the time, silent otherwise.
inline void adaptThreshold(float& theta, float meanActivity, const LevelParams& lp, float target) {
    if (lp.homeostasis_rate > 0.0f)
        theta = std::clamp(theta + lp.homeostasis_rate * (meanActivity - target), lp.theta_min, lp.theta_max);
}

// Writes one cell's activated state; returns its drive (mean activated value).
// With keep < C, only the `keep` most strongly driven channels stay active: competition
// among the neurons inside one cell (ties go to the lower channel).
template <uint32_t C>
inline float activateCell(const float* in, float* out, float theta, uint32_t keep = C) {
    float sum = 0.0f;
    for (uint32_t c = 0; c < C; ++c) out[c] = activate(in[c] - theta);
    if (keep < C) {
        for (uint32_t c = 0; c < C; ++c) {
            if (out[c] == 0.0f) continue;
            uint32_t stronger = 0;
            for (uint32_t d = 0; d < C; ++d)
                if (in[d] > in[c] || (in[d] == in[c] && d < c)) ++stronger;
            if (stronger >= keep) out[c] = 0.0f;
        }
    }
    for (uint32_t c = 0; c < C; ++c) sum += out[c];
    return sum / float(C);
}

// 1D cells: no competition (lines carry sequences intact, spec Section 3C).
template <uint32_t C>
inline void finishCell(const float* in, float* out, float& theta, const LevelParams& lp, float target) {
    adaptThreshold(theta, activateCell<C>(in, out, theta), lp, target);
}

// Scales a cell's output (divisive fatigue) and returns its new mean.
template <uint32_t C>
inline float scaleCell(float* out, float factor) {
    float sum = 0.0f;
    for (uint32_t c = 0; c < C; ++c) sum += (out[c] *= factor);
    return sum / float(C);
}

// Lateral inhibition as local competition (spec Section 3C): a cell keeps its
// activity only if fewer than `winners` of its neighbours are driven harder
// (ties go to the lower index); otherwise its neighbours suppress it. Allowing a
// few winners per neighbourhood lets small groups of neighbouring cells fire
// together, which Hebbian learning needs to bind them. Returns the final mean.
template <uint32_t C>
inline float competeCell(float* out, float ownDrive, size_t own, const size_t* rivals, uint32_t rivalCount,
                         const float* drive, uint32_t winners, float sigma = 0.0f, float fireLevel = 0.0f,
                         float fireGain = 0.0f) {
    uint32_t stronger = 0;
    bool wins = ownDrive > 0.0f;
    for (uint32_t r = 0; wins && r < rivalCount; ++r) {
        const float d = drive[rivals[r]];
        if (d > ownDrive || (d == ownDrive && rivals[r] < own))
            if (++stronger >= winners) wins = false;
    }
    if (wins) {
        if (fireGain > 0.0f) {
            // Rate coding (a neuron's firing curve): silent below the firing level, rising
            // steeply above it, saturating at 1. Signals keep their strength between fields,
            // and fatigue lowers the rate smoothly instead of switching the cell on and off.
            float sum = 0.0f;
            for (uint32_t c = 0; c < C; ++c)
                sum += (out[c] = std::min(1.0f, fireGain * std::max(0.0f, out[c] - fireLevel)));
            return sum / float(C);
        }
        if (fireLevel > 0.0f) {
            // Firing: a winner whose strongest channel reaches the firing level fires at full
            // strength (its pattern across channels kept, strongest channel = 1), like a
            // neuron that spikes; below the level it stays silent instead of passing on a
            // faint copy of its input.
            float strongest = 0.0f;
            for (uint32_t c = 0; c < C; ++c) strongest = std::max(strongest, out[c]);
            if (strongest >= fireLevel) {
                float sum = 0.0f;
                for (uint32_t c = 0; c < C; ++c) sum += (out[c] /= strongest);
                return sum / float(C);
            }
            for (uint32_t c = 0; c < C; ++c) out[c] = 0.0f;
            return 0.0f;
        }
        if (sigma > 0.0f) {
            // Output normalization (divisive): winners fire at a consistent strength.
            float strongest = 0.0f;
            for (uint32_t c = 0; c < C; ++c) strongest = std::max(strongest, out[c]);
            const float scale = (1.0f + sigma) / (sigma + strongest);
            float sum = 0.0f;
            for (uint32_t c = 0; c < C; ++c) sum += (out[c] = std::min(1.0f, out[c] * scale));
            return sum / float(C);
        }
        return ownDrive;
    }
    for (uint32_t c = 0; c < C; ++c) out[c] = 0.0f;
    return 0.0f;
}

// One block of learned connections from cell j to cell i (spec Section 5A):
//   dW[a][b] = rate * ( (post[a] - c*avgPost)*(pre[b] - c*avgPre)        association (covariance)
//                     + lambda*(prePrev[b]*post[a] - pre[b]*postPrev[a])  order: j before i
//                     - post[a]^2 * W[a][b] )                             Oja normalization
// c blends plain Hebbian (0) and covariance (1) association. Strengths stay
// non-negative; a connection's sign comes from its source cell.
// Returns the total absolute change applied to the block.
template <uint32_t C>
// `room` (optional, one value per output channel in [0, 1]) scales strengthening only: soft
// bounds, so a channel whose plastic budget is already full of memories learns new ones slowly.
// `predicted` (optional, per output channel) is what the cell's plastic inputs already predict;
// association then learns only the unpredicted part of the cell's activity (delta rule).
inline double hebbianBlock(float* W, const float* post, const float* postPrev, const float* pre,
                           const float* prePrev, float avgPost, float avgPre, float rate, float lambda,
                           float oja, const float* room, const float* predicted, const float* assocPost,
                           const float* assocPre, float hetero, const float* preRoom) {
    double change = 0.0;
    for (uint32_t a = 0; a < C; ++a) {
        const float pa = post[a], qa = postPrev[a];
        if (pa == 0.0f && qa == 0.0f && assocPost[a] == 0.0f) continue;
        float* row = W + size_t(a) * C;
        const float da = assocPost[a] - avgPost - (predicted ? predicted[a] : 0.0f);
        const float up = room ? room[a] : 1.0f;
        for (uint32_t b = 0; b < C; ++b) {
            // Heterosynaptic depression: a silent input of an active cell is weakened by
            // `hetero` times the covariance term (1 = full covariance, 0 = only active inputs
            // change). Silent inputs include other memories' cells, so this erases them.
            const float preTerm = assocPre[b] == 0.0f ? hetero * (0.0f - avgPre) : assocPre[b] - avgPre;
            const float dw =
                da * preTerm + lambda * (prePrev[b] * pa - pre[b] * qa) - oja * pa * pa * row[b];
            const float grow = up * (preRoom ? preRoom[b] : 1.0f);
            const float updated = std::max(0.0f, row[b] + rate * (dw > 0.0f ? grow * dw : dw));
            change += std::fabs(updated - row[b]);
            row[b] = updated;
        }
    }
    return change;
}

inline bool anyActive(const float* a, const float* b, uint32_t n) {
    for (uint32_t c = 0; c < n; ++c)
        if (a[c] != 0.0f || b[c] != 0.0f) return true;
    return false;
}

// Event-driven updates (spec Section 3C): a silent source contributes nothing, so it is skipped.
inline bool isSilent(const float* x, size_t n) {
    for (size_t i = 0; i < n; ++i)
        if (x[i] != 0.0f) return false;
    return true;
}

void fillNormal(AVec<float>& w, Rng& rng, double stddev) {
    for (float& v : w) v = float(rng.normal() * stddev);
}

} // namespace

NeuralCellularMatrix::NeuralCellularMatrix(const Config& cfg) : cfg_(cfg) {
    if (cfg.field_dim < 2 || cfg.sheet_dim < 2 || cfg.line_len < 2)
        throw std::invalid_argument("field, sheet and line sizes must all be at least 2");

    N_ = cfg.field_dim;
    S_ = cfg.sheet_dim;
    L_ = cfg.line_len;
    Vf_ = cfg.voxelsPerField();
    V_ = cfg.voxels();
    SS_ = cfg.sheetCellsPerVoxel();
    Q_ = cfg.sheetCells();
    P_ = cfg.lineCells();

    s1_.allocate(P_, C1);
    lineQuietCur_.assign(Q_, 1);
    lineQuietNext_.assign(Q_, 1);
    sheetQuietCur_.assign(V_, 1);
    sheetQuietNext_.assign(V_, 1);
    sheetSkip_.assign(V_, 0);
    s2_.allocate(Q_, C2);
    s3_.allocate(V_, C3);
    drive2_.assign(Q_, 0.0f);
    drive3_.assign(V_, 0.0f);
    diagPlastic3_.assign(V_, 0.0f);
    diagInput3_.assign(V_, 0.0f);
    fatigue2_.assign(Q_, 0.0f);
    voxelGain_.assign(V_, 1.0f);
    if (cfg.learning.sheet_rate > 0.0f) P2_.assign(Q_ * C2 * C2, 0.0f);
    fatigue3_.assign(V_, 0.0f);
    average3_.assign(V_, cfg.target_activity);
    averageN3_.assign(V_, cfg.target_activity);
    trace3_.assign(V_ * C3, 0.0f);
    orderTrace3_.assign(V_ * C3, 0.0f);
    resource3_.assign(V_ * C3, 1.0f);
    membrane3_.assign(V_ * C3, 0.0f);
    diagSource3_.assign(V_ * kSources, 0.0f);
    inhibW3_.assign(V_, 0.0f);
    pool3_.assign(V_, 0.0f);
    inhibSignal3_.assign(V_, 0.0f);

    initInhibitory();
    initSharedRules();
    initPositionWeights();
    initVoxelWeights();
    initSurfaces();
}

void NeuralCellularMatrix::initInhibitory() {
    inhib2_.resize(Q_);
    inhib3_.resize(V_);
    for (size_t q = 0; q < Q_; ++q)
        inhib2_[q] = hashUniform(cfg_.seed, kStreamInhibitory2, q) < cfg_.inhibitory_share;
    for (size_t v = 0; v < V_; ++v)
        inhib3_[v] = hashUniform(cfg_.seed, kStreamInhibitory3, v) < cfg_.inhibitory_share;
}

void NeuralCellularMatrix::initSharedRules() {
    const StartingRule& r = cfg_.rule;

    W1_.assign(3 * C1 * C1, 0.0f);
    setIdentity(W1_.data() + 0 * C1 * C1, C1, r.line_carry); // offset -1: from the left neighbour

    W2_.assign(9 * C2 * C2, 0.0f);
    for (uint32_t o = 0; o < 9; ++o)
        setIdentity(W2_.data() + size_t(o) * C2 * C2, C2, o == 4 ? r.sheet_self : r.sheet_neighbour);
}

void NeuralCellularMatrix::initPositionWeights() {
    // Fixed random projections, scaled by 1/sqrt(fan-in) so summaries and drives
    // keep a similar magnitude at every size (spec Section 2B).
    U1_.resize(size_t(L_) * C2 * C1);
    D1_.resize(size_t(C1) * C2);
    U2_.resize(SS_ * C3 * C2);
    D2_.resize(SS_ * C2 * C3);

    Rng u1(mix64(cfg_.seed ^ kStreamPositionUp1));
    Rng d1(mix64(cfg_.seed ^ kStreamPositionDown1));
    Rng u2(mix64(cfg_.seed ^ kStreamPositionUp2));
    Rng d2(mix64(cfg_.seed ^ kStreamPositionDown2));
    fillNormal(U1_, u1, 1.0 / std::sqrt(double(L_) * C1));
    fillNormal(D1_, d1, 1.0 / std::sqrt(double(C2)));
    fillNormal(U2_, u2, 1.0 / std::sqrt(double(SS_) * C2));
    fillNormal(D2_, d2, 1.0 / std::sqrt(double(C3)));
}

void NeuralCellularMatrix::initVoxelWeights() {
    // Every learned connection = fixed scaffold + plastic part. The scaffold is a scaled
    // one-to-one channel map (StartingRule strengths) applied on the fly and never stored
    // or changed; these arrays hold only the plastic part, which starts at zero.
    const uint32_t K = cfg_.long_range_links;

    W3_.assign(V_ * 27 * C3 * C3, 0.0f);
    if (cfg_.learning.consolidation_rate > 0.0f) {
        S3_.assign(V_ * 27 * C3 * C3, 0.0f);
        SL_.assign(V_ * cfg_.long_range_links * C3 * C3, 0.0f);
        SH_.assign(V_ * 3 * C3 * C3, 0.0f);
    }
    WL_.assign(V_ * K * C3 * C3, 0.0f);
    H_.assign(V_ * 3 * C3 * C3, 0.0f);
    M2_.assign(V_ * C2 * C2, 0.0f);
    lrTarget_.resize(V_ * K);

    // Long-range targets: random voxels in the same field, fixed for life (spec Section 3C).
    const int64_t voxels = int64_t(V_);
#pragma omp parallel for schedule(dynamic, 256)
    for (int64_t vi = 0; vi < voxels; ++vi) {
        const size_t v = size_t(vi);
        const size_t fieldBase = (v / Vf_) * Vf_;
        for (uint32_t l = 0; l < K; ++l) {
            size_t t = v;
            uint64_t attempt = 0;
            while (t == v && Vf_ > 1) {
                const double u = hashUniform(cfg_.seed, kStreamLongRange, (uint64_t(v) * K + l) * 64 + attempt++);
                t = fieldBase + std::min(size_t(u * double(Vf_)), Vf_ - 1);
            }
            lrTarget_[v * K + l] = uint32_t(t);
        }
    }

    // Pattern-separating 4D sources: random positions within a field, fixed for life. With
    // link4d_spread_scaled the count grows with the field's side length (link4d_spread is the
    // count at side 12): the active share of a field falls as 1/side (activity enters through a
    // face), so each target then sees the same expected number of active sources at any size.
    spreadK_ = cfg_.link4d_spread;
    if (cfg_.link4d_spread_scaled > 0.5f && spreadK_ > 0)
        spreadK_ = std::max<uint32_t>(1, uint32_t(double(cfg_.link4d_spread) * double(N_) / 12.0 + 0.5));
    // Input-field depth sources: each interior voxel of the Input field (x > 0) gets fixed
    // random sources on the field's sensory face (x = 0). The face's active share is the same
    // at every size, so the count is not scaled (growth-safe).
    const uint32_t KD = cfg_.input_depth_spread;
    inputDepthPos_.assign(Vf_ * KD, 0);
    for (size_t v = 0; v < Vf_ * KD; ++v) {
        const double u = hashUniform(cfg_.seed, kStreamInputDepth, uint64_t(v));
        inputDepthPos_[v] = uint32_t(std::min(size_t(u * double(size_t(N_) * N_)), size_t(N_) * N_ - 1));
    }
    const uint32_t S4 = spreadK_;
    spreadPos_.assign(V_ * S4, 0);
    for (size_t v = 0; v < V_; ++v)
        for (uint32_t l = 0; l < S4; ++l) {
            const double u = hashUniform(cfg_.seed, kStreamLink4dSpread, uint64_t(v) * S4 + l);
            spreadPos_[v * S4 + l] = uint32_t(std::min(size_t(u * double(Vf_)), Vf_ - 1));
        }
}

void NeuralCellularMatrix::initSurfaces() {
    const size_t lines = cfg_.surfaceLines();
    sensoryQ_.resize(lines);
    motorQ_.resize(lines);
    sensoryDrive_.assign(lines, 0);
    motorDrive_.assign(lines, 0);

    // Surface line index = ((z * N + y) * S + sy) * S + sx over one face of a field.
    for (uint32_t z = 0; z < N_; ++z)
        for (uint32_t y = 0; y < N_; ++y)
            for (uint32_t cell = 0; cell < SS_; ++cell) {
                const size_t s = (size_t(z) * N_ + y) * SS_ + cell;
                const size_t vin = voxelIndex(uint32_t(Field::Input), 0, y, z);
                const size_t vout = voxelIndex(uint32_t(Field::Output), N_ - 1, y, z);
                sensoryQ_[s] = uint32_t(vin * SS_ + cell);
                motorQ_[s] = uint32_t(vout * SS_ + cell);
            }
}

void NeuralCellularMatrix::setSensoryInput(const std::vector<uint32_t>& active_lines) {
    std::fill(sensoryDrive_.begin(), sensoryDrive_.end(), uint8_t(0));
    for (uint32_t s : active_lines)
        if (s < sensoryDrive_.size()) sensoryDrive_[s] = 1;
    sensoryOn_ = true;
}

void NeuralCellularMatrix::clearSensoryInput() { sensoryOn_ = false; }

void NeuralCellularMatrix::clampMotor(const std::vector<uint32_t>& active_lines) {
    std::fill(motorDrive_.begin(), motorDrive_.end(), uint8_t(0));
    for (uint32_t s : active_lines)
        if (s < motorDrive_.size()) motorDrive_[s] = 1;
    motorOn_ = true;
}

void NeuralCellularMatrix::releaseMotor() { motorOn_ = false; }

std::vector<uint32_t> NeuralCellularMatrix::readMotorExit() const {
    std::vector<uint32_t> active;
    const float level = cfg_.level1.active_level;
    for (size_t s = 0; s < motorQ_.size(); ++s) {
        const float* cell = s1_.cur.data() + (size_t(motorQ_[s]) * L_ + (L_ - 1)) * C1;
        float sum = 0.0f;
        for (uint32_t c = 0; c < C1; ++c) sum += cell[c];
        if (sum / float(C1) >= level) active.push_back(uint32_t(s));
    }
    return active;
}

void NeuralCellularMatrix::applySurfaceClamps() {
    if (sensoryOn_) {
        for (size_t s = 0; s < sensoryQ_.size(); ++s) {
            float* entry = s1_.next.data() + size_t(sensoryQ_[s]) * L_ * C1; // position 0
            const float v = sensoryDrive_[s] ? 1.0f : 0.0f;
            for (uint32_t c = 0; c < C1; ++c) entry[c] = v;
            const size_t q = sensoryQ_[s];
            lineQuietNext_[q] = isSilent(s1_.next.data() + q * L_ * C1, size_t(L_) * C1);
        }
    }
    if (motorOn_) {
        for (size_t s = 0; s < motorQ_.size(); ++s) {
            float* exitCell = s1_.next.data() + (size_t(motorQ_[s]) * L_ + (L_ - 1)) * C1;
            const float v = motorDrive_[s] ? 1.0f : 0.0f;
            for (uint32_t c = 0; c < C1; ++c) exitCell[c] = v;
            const size_t q = motorQ_[s];
            lineQuietNext_[q] = isSilent(s1_.next.data() + q * L_ * C1, size_t(L_) * C1);
        }
    }
}

// 1D level: every line carries its sequence one cell per tick toward the exit,
// driven from above by its parent sheet cell (spec Sections 2B, 6B).
void NeuralCellularMatrix::step1D() {
    const float* s1 = s1_.cur.data();
    const float* s2 = s2_.cur.data();
    float* out = s1_.next.data();
    float* theta = s1_.theta.data();
    const float gd = cfg_.downward_gain;
    const LevelParams lp = cfg_.level1;
    const float target = cfg_.target_activity;
    const uint32_t L = L_;

    const int64_t lines = int64_t(Q_);
#pragma omp parallel for schedule(dynamic, 256)
    for (int64_t qi = 0; qi < lines; ++qi) {
        const size_t q = size_t(qi);
        const float* parent = s2 + q * C2;
        const float* line = s1 + q * L * C1;
        float* lineOut = out + q * L * C1;
        // A silent line under a silent parent stays silent: skip the arithmetic. Quiet flags
        // (kept per buffer) let such lines be skipped without reading them, and without
        // rewriting zeros that are already there; results are identical.
        if (lineQuietCur_[q] && isSilent(parent, C2)) {
            if (!lineQuietNext_[q]) {
                std::fill(lineOut, lineOut + size_t(L) * C1, 0.0f);
                lineQuietNext_[q] = 1;
            }
            if (lp.homeostasis_rate > 0.0f)
                for (uint32_t k = 0; k < L; ++k) adaptThreshold(theta[q * L + k], 0.0f, lp, target);
            continue;
        }
        for (uint32_t k = 0; k < L; ++k) {
            float in[C1] = {};
            for (int o = -1; o <= 1; ++o) {
                const int kk = int(k) + o;
                if (kk < 0 || kk >= int(L)) continue;
                matvecAdd(W1_.data() + size_t(o + 1) * C1 * C1, line + size_t(kk) * C1, in, C1, C1, 1.0f);
            }
            // The parent writes into the entry cell only; the carry moves what entered
            // along the line, so each line holds the last L entries in order.
            if (k == 0) matvecAdd(D1_.data(), parent, in, C1, C2, gd);
            finishCell<C1>(in, lineOut + size_t(k) * C1, theta[q * L + k], lp, target);
        }
        lineQuietNext_[q] = isSilent(lineOut, size_t(L) * C1);
    }

    applySurfaceClamps();
    s1_.swap();
    lineQuietCur_.swap(lineQuietNext_);
}

// 2D level: lateral interaction within each sheet, a summary from each cell's own
// 1D line, drive from the parent voxel, and the voxel's learned modulation.
void NeuralCellularMatrix::step2D() {
    // Fatigue at full strength while encoding and in silence (it ends activity that outlasts
    // its input); in recall mode (M = 0) it is scaled to fatigue_recall so a recalled memory
    // can settle instead of wearing itself out.
    const float fatigueMode = std::clamp(cfg_.fatigue_recall, 0.0f, 1.0f) +
                              (1.0f - std::clamp(cfg_.fatigue_recall, 0.0f, 1.0f)) * std::clamp(modulator_, 0.0f, 1.0f);
    // Divisive fatigue: a tired cell fires more slowly instead of being silenced, so a steady
    // input is never switched off (subtractive fatigue silenced whole fields under held input).
    // fatigue_divisive 2 = divisive only while sensory input is present (steady input keeps
    // cells firing) and subtractive in silence (activity that outlasts its input still ends).
    const bool divisiveFatigue = cfg_.fatigue_divisive2 > 0.5f ||
                                 (cfg_.fatigue_divisive > 1.5f ? sensoryOn_ : cfg_.fatigue_divisive > 0.5f);
    // Capped fatigue: the threshold rise is limited to fatigue_cap, enough to end the weak
    // activity that outlasts its input but not to silence cells a present input drives.
    const float fatigueCap = cfg_.fatigue_cap;
    auto fatigueShift = [fatigueCap](float shift) { return fatigueCap > 0.0f ? std::min(shift, fatigueCap) : shift; };
    const float* s1 = s1_.cur.data();
    const float* s2 = s2_.cur.data();
    const float* s3 = s3_.cur.data();
    float* out = s2_.next.data();
    float* theta = s2_.theta.data();
    const float gd = cfg_.downward_gain;
    const float rec2 = 1.0f - std::clamp(cfg_.learning.encoding_suppression, 0.0f, 1.0f) *
                                  std::clamp(modulator_, 0.0f, 1.0f);
    const LevelParams lp = cfg_.level2;
    const float target = cfg_.target_activity;
    const float inh = -cfg_.inhibitory_strength;
    const uint32_t S = S_, L = L_;
    const size_t SS = SS_;

    // Event-driven skipping of whole sheets: a sheet that is silent, whose lines are all
    // quiet and whose parent voxel is silent receives no input at all, so every cell's drive
    // and output are exactly 0; only its threshold and fatigue drift, updated in pass 2.
    const int64_t voxels = int64_t(V_);
#pragma omp parallel for schedule(static)
    for (int64_t vi = 0; vi < voxels; ++vi) {
        const size_t v = size_t(vi);
        bool quiet = sheetQuietCur_[v] && isSilent(s3 + v * C3, C3);
        for (size_t c = 0; quiet && c < SS; ++c) quiet = lineQuietCur_[v * SS + c] != 0;
        sheetSkip_[v] = quiet ? 1 : 0;
    }

    const int64_t cells = int64_t(Q_);
#pragma omp parallel for schedule(dynamic, 256)
    for (int64_t qi = 0; qi < cells; ++qi) {
        const size_t q = size_t(qi);
        const size_t v = q / SS;
        if (sheetSkip_[v]) {
            if (!sheetQuietNext_[v]) std::fill(out + q * C2, out + (q + 1) * C2, 0.0f);
            drive2_[q] = 0.0f;
            continue;
        }
        const size_t cell = q % SS;
        const int sy = int(cell / S), sx = int(cell % S);

        float in[C2] = {};
        float neighbours[C2] = {}; // summed excitatory neighbour state (input to sheet learning)
        bool anyNeighbour = false;
        for (int dy = -1; dy <= 1; ++dy)
            for (int dx = -1; dx <= 1; ++dx) {
                const int ny = sy + dy, nx = sx + dx;
                if (ny < 0 || nx < 0 || ny >= int(S) || nx >= int(S)) continue;
                const size_t qn = v * SS + size_t(ny) * S + size_t(nx);
                const uint32_t o = uint32_t((dy + 1) * 3 + (dx + 1));
                if (isSilent(s2 + qn * C2, C2)) continue; // silent sources contribute nothing
                // Self-persistence is not a synapse; lateral inputs carry the sender's sign.
                const float sign = (o == 4 || !inhib2_[qn]) ? 1.0f : inh;
                matvecAdd(W2_.data() + size_t(o) * C2 * C2, s2 + qn * C2, in, C2, C2, sign);
                if (o != 4 && !inhib2_[qn]) {
                    addScaled(s2 + qn * C2, neighbours, C2, 1.0f);
                    anyNeighbour = true;
                }
            }
        // Sheet learning: a learned block from the cell's summed neighbourhood (same Hebbian rule
        // as the 3D level), transmitted at the encoding/recall mode like other learned links.
        if (!P2_.empty() && anyNeighbour) matvecAdd(P2_.data() + q * C2 * C2, neighbours, in, C2, C2, rec2);
        if (!isSilent(s2 + q * C2, C2)) matvecAdd(M2_.data() + v * C2 * C2, s2 + q * C2, in, C2, C2, 1.0f);
        // Upward summary with divisive normalization: scaled by 1/sqrt(active line cells), so
        // a streamed character (one active cell per line) and a held one (a full line) drive
        // the sheet cell in the same useful range.
        {
            float up[C2] = {};
            uint32_t activeCells = 0;
            // A quiet line contributes nothing: its flag spares reading it.
            for (uint32_t k = 0; k < (lineQuietCur_[q] ? 0u : L); ++k) {
                const float* cell = s1 + (q * L + k) * C1;
                bool active = false;
                for (uint32_t c = 0; c < C1; ++c) active = active || cell[c] != 0.0f;
                if (!active) continue;
                ++activeCells;
                matvecAdd(U1_.data() + size_t(k) * C2 * C1, cell, up, C2, C1, 1.0f);
            }
            if (activeCells > 0) {
                const float norm = cfg_.normalize_upward > 0.0f ? std::sqrt(float(activeCells)) : 1.0f;
                const float scale = cfg_.line_upward_gain / norm;
                for (uint32_t c = 0; c < C2; ++c) in[c] += scale * up[c];
            }
        }
        if (!isSilent(s3 + v * C3, C3)) matvecAdd(D2_.data() + cell * C2 * C3, s3 + v * C3, in, C2, C3, gd);

        drive2_[q] = activateCell<C2>(in, out + q * C2,
                                      theta[q] + (divisiveFatigue ? 0.0f : fatigueShift(lp.fatigue_gain * fatigueMode * fatigue2_[q])),
                                      cfg_.channel_winners2);
    }

    // Pass 2: local competition within each sheet (3 x 3 neighbourhood), then homeostasis.
    const float* drive = drive2_.data();
#pragma omp parallel for schedule(dynamic, 256)
    for (int64_t qi = 0; qi < cells; ++qi) {
        const size_t q = size_t(qi);
        const size_t v = q / SS;
        if (sheetSkip_[v]) { // exactly what the full path gives a cell with no drive
            adaptThreshold(theta[q], 0.0f, lp, target);
            fatigue2_[q] += (0.0f - fatigue2_[q]) / std::max(1.0f, lp.fatigue_tau);
            continue;
        }
        const size_t cell = q % SS;
        const int sy = int(cell / S), sx = int(cell % S);
        size_t rivals[8];
        uint32_t n = 0;
        for (int dy = -1; dy <= 1; ++dy)
            for (int dx = -1; dx <= 1; ++dx) {
                const int ny = sy + dy, nx = sx + dx;
                if ((dy == 0 && dx == 0) || ny < 0 || nx < 0 || ny >= int(S) || nx >= int(S)) continue;
                rivals[n++] = v * SS + size_t(ny) * S + size_t(nx);
            }
        float final = competeCell<C2>(out + q * C2, drive[q], q, rivals, n, drive, cfg_.winners2, cfg_.output_sigma,
                                         cfg_.fire_threshold2, cfg_.fire_gain2);
        if (divisiveFatigue && final > 0.0f) final = scaleCell<C2>(out + q * C2, 1.0f / (1.0f + lp.fatigue_gain * fatigueMode * fatigue2_[q]));
        adaptThreshold(theta[q], final, lp, target);
        // Fatigue follows the activity of the channels that fire (the mean over all channels
        // understates it by C / channel_winners, so fatigue could never build up).
        const float firing2 = final * float(C2) / float(std::clamp<uint32_t>(cfg_.channel_winners2, 1, C2));
        fatigue2_[q] += (firing2 - fatigue2_[q]) / std::max(1.0f, lp.fatigue_tau);
    }
#pragma omp parallel for schedule(static)
    for (int64_t vi = 0; vi < voxels; ++vi) {
        const size_t v = size_t(vi);
        sheetQuietNext_[v] = sheetSkip_[v] ? 1 : (isSilent(out + v * SS * C2, SS * C2) ? 1 : 0);
    }
    s2_.swap();
    sheetQuietCur_.swap(sheetQuietNext_);
}

// 3D level: learned neighbourhood, long-range links, the 4D link to the other
// three fields, and a summary from each voxel's own 2D sheet.
void NeuralCellularMatrix::step3D() {
    // Fatigue at full strength while encoding and in silence (it ends activity that outlasts
    // its input); in recall mode (M = 0) it is scaled to fatigue_recall so a recalled memory
    // can settle instead of wearing itself out.
    const float fatigueMode = std::clamp(cfg_.fatigue_recall, 0.0f, 1.0f) +
                              (1.0f - std::clamp(cfg_.fatigue_recall, 0.0f, 1.0f)) * std::clamp(modulator_, 0.0f, 1.0f);
    // Divisive fatigue: a tired cell fires more slowly instead of being silenced, so a steady
    // input is never switched off (subtractive fatigue silenced whole fields under held input).
    // fatigue_divisive 2 = divisive only while sensory input is present (steady input keeps
    // cells firing) and subtractive in silence (activity that outlasts its input still ends).
    const bool divisiveFatigue = cfg_.fatigue_divisive > 1.5f ? sensoryOn_ : cfg_.fatigue_divisive > 0.5f;
    // Capped fatigue: the threshold rise is limited to fatigue_cap, enough to end the weak
    // activity that outlasts its input but not to silence cells a present input drives.
    const float fatigueCap = cfg_.fatigue_cap;
    auto fatigueShift = [fatigueCap](float shift) { return fatigueCap > 0.0f ? std::min(shift, fatigueCap) : shift; };
    const float* s2 = s2_.cur.data();
    const float* s3 = s3_.cur.data();
    float* out = s3_.next.data();
    float* theta = s3_.theta.data();
    const float gu = cfg_.upward_gain;
    const LevelParams lp = cfg_.level3;
    const float target = cfg_.target_activity;
    const float inh = -cfg_.inhibitory_strength;
    const float avgTau = std::max(1.0f, cfg_.learning.average_tau);
    const float traceTau = cfg_.learning.trace_tau > 0.0f ? std::max(1.0f, cfg_.learning.trace_tau) : 0.0f;
    const float orderTau = cfg_.learning.order_tau > 0.0f ? std::max(1.0f, cfg_.learning.order_tau) : 0.0f;
    // Learned excitatory connections transmit less in encoding mode (high modulator).
    const float rec = 1.0f - std::clamp(cfg_.learning.encoding_suppression, 0.0f, 1.0f) *
                                 std::clamp(modulator_, 0.0f, 1.0f);
    // The learned part of the 4D link is an association too; its scaffold is the input path.
    const float rec4 = 1.0f - std::clamp(cfg_.learning.encoding_suppression_4d, 0.0f, 1.0f) *
                                  std::clamp(modulator_, 0.0f, 1.0f);
    const StartingRule& r = cfg_.rule;
    const uint32_t N = N_;
    const uint32_t K = cfg_.long_range_links;
    const size_t SS = SS_;
    const bool depression = cfg_.learning.depression_use > 0.0f;
    const bool learnedInhibition = cfg_.learning.istdp_rate > 0.0f;

    const int64_t voxels = int64_t(V_);
#pragma omp parallel for schedule(dynamic, 256)
    for (int64_t vi = 0; vi < voxels; ++vi) {
        const size_t v = size_t(vi);
        const uint32_t x = uint32_t(v % N);
        const uint32_t y = uint32_t((v / N) % N);
        const uint32_t z = uint32_t((v / (size_t(N) * N)) % N);
        const uint32_t f = uint32_t(v / Vf_);

        float in[C3] = {};
        float pl[C3] = {};  // learned recurrent part of the input (within the field)
        float pl4[C3] = {}; // learned part of the 4D link (between fields)
        auto sumIn = [&]() {
            float t = 0.0f;
            for (uint32_t c = 0; c < C3; ++c) t += in[c];
            return t;
        };
        // Learned connections transmit the source's activity times its transmitter resource.
        float depressed[C3];
        auto learnedSource = [&](size_t sv, const float* src) -> const float* {
            if (!depression) return src;
            const float* r = resource3_.data() + sv * C3;
            for (uint32_t c = 0; c < C3; ++c) depressed[c] = src[c] * r[c];
            return depressed;
        };
        // Gain control scales incoming signals only: per voxel (local) or per field.
        const float afferentGain = cfg_.agc_local > 0.5f ? voxelGain_[v] : fieldGain_[f];
        const float* w = W3_.data() + v * 27 * C3 * C3;
        float poolSum = 0.0f;
        uint32_t poolCount = 0;
        for (int dz = -1; dz <= 1; ++dz)
            for (int dy = -1; dy <= 1; ++dy)
                for (int dx = -1; dx <= 1; ++dx) {
                    const int nx = int(x) + dx, ny = int(y) + dy, nz = int(z) + dz;
                    if (nx < 0 || ny < 0 || nz < 0 || nx >= int(N) || ny >= int(N) || nz >= int(N)) continue;
                    const size_t vn = voxelIndex(f, uint32_t(nx), uint32_t(ny), uint32_t(nz));
                    const uint32_t o = uint32_t((dz + 1) * 9 + (dy + 1) * 3 + (dx + 1));
                    const float* src = s3 + vn * C3;
                    ++poolCount;
                    if (isSilent(src, C3)) continue; // silent sources contribute nothing
                    for (uint32_t c = 0; c < C3; ++c) poolSum += src[c];
                    if (o == 13) {
                        addScaled(src, in, C3, r.voxel_self); // self-persistence: fixed
                    } else if (inhib3_[vn]) {
                        addScaled(src, in, C3, inh * r.voxel_neighbour); // inhibitory: fixed scaffold only
                    } else {
                        addScaled(src, in, C3, r.voxel_neighbour);                // scaffold
                        const float* ls = learnedSource(vn, src);
                        matvecAdd(w + size_t(o) * C3 * C3, ls, pl, C3, C3, rec); // plastic memory part
                        if (!S3_.empty()) matvecAdd(S3_.data() + (v * 27 + o) * C3 * C3, ls, pl, C3, C3, rec);
                    }
                }

        const float cLocal = sumIn();
        for (uint32_t l = 0; l < K; ++l) {
            const size_t t = lrTarget_[v * K + l];
            const float* src = s3 + t * C3;
            if (isSilent(src, C3)) continue;
            if (inhib3_[t]) {
                addScaled(src, in, C3, inh * r.long_range);
            } else {
                addScaled(src, in, C3, r.long_range);
                const float* ls = learnedSource(t, src);
                matvecAdd(WL_.data() + (v * K + l) * C3 * C3, ls, pl, C3, C3, rec);
                if (!SL_.empty()) matvecAdd(SL_.data() + (v * K + l) * C3 * C3, ls, pl, C3, C3, rec);
            }
        }

        const float cLong = sumIn();
        // The 4D link is the path input takes between fields, so encoding mode does not
        // suppress it: acetylcholine turns down a region's internal loops, not its input.
        uint32_t gi = 0;
        for (uint32_t g = 0; g < kFields; ++g) {
            if (g == f) continue;
            const size_t vg = voxelIndex(g, x, y, z);
            const float* src = s3 + vg * C3;
            if (isSilent(src, C3)) {
                ++gi;
                continue;
            }
            const uint32_t S4 = spreadK_;
            const float spreadShare = (g < f && S4 > 0) ? std::clamp(r.link4d_spread_share, 0.0f, 1.0f) : 0.0f;
            const float link = g < f ? r.link4d * (1.0f - spreadShare) : r.link4d_backward; // feedforward vs feedback
            // The learned part keeps the same asymmetry: feedback transmits at the
            // backward/forward ratio, or learned feedback would rebuild the loops.
            const float plasticScale =
                g < f ? 1.0f : (r.link4d > 0.0f ? std::min(1.0f, r.link4d_backward / r.link4d) : 0.0f);
            // Gain control boosts feedforward input only. Feedback from later fields is never
            // amplified: the fields also form loops through the 4D link, and amplifying the
            // feedback side made them sustain each other.
            const float gain = g < f ? afferentGain : 1.0f;
            if (inhib3_[vg]) {
                addScaled(src, in, C3, gain * inh * link);
            } else {
                addScaled(src, in, C3, gain * link);
                const float* ls = learnedSource(vg, src);
                matvecAdd(H_.data() + (v * 3 + gi) * C3 * C3, ls, pl4, C3, C3, gain * plasticScale * rec4);
                if (!SH_.empty())
                    matvecAdd(SH_.data() + (v * 3 + gi) * C3 * C3, ls, pl4, C3, C3, gain * plasticScale * rec4);
            }
            ++gi;
        }
        const float c4D = sumIn();
        // Input-field depth: interior voxels hear random voxels of the sensory face (feedforward).
        if (f == 0 && x > 0 && cfg_.input_depth_spread > 0) {
            const uint32_t KD = cfg_.input_depth_spread;
            const float each = r.input_depth_gain;
            for (uint32_t l = 0; l < KD; ++l) {
                const uint32_t yz = inputDepthPos_[v * KD + l];
                const size_t vs = voxelIndex(0, 0, yz % N, yz / N);
                const float* src = s3 + vs * C3;
                if (isSilent(src, C3)) continue;
                addScaled(src, in, C3, inhib3_[vs] ? inh * each : each);
            }
        }
        const float cDepth = sumIn();
        // Pattern-separating feedforward sources (scaffold only), from every earlier field.
        if (spreadK_ > 0 && f > 0) {
            const uint32_t S4 = spreadK_;
            // Weight per source is set by the reference count, so the expected drive (active
            // sources x weight) is the same at every size when the count is scaled.
            const float each = afferentGain * r.link4d * std::clamp(r.link4d_spread_share, 0.0f, 1.0f) *
                               r.link4d_spread_gain / float(std::max<uint32_t>(1, cfg_.link4d_spread));
            for (uint32_t g = 0; g < f; ++g)
                // Only the Input field's activity is a thin slab whose active share falls with
                // size; fields fed by the projection fill their volume at a constant share, so
                // sources from them use the reference count (keeps density size-invariant).
                for (uint32_t l = 0; l < (g == 0 ? S4 : std::min<uint32_t>(S4, cfg_.link4d_spread)); ++l) {
                    const size_t vs = size_t(g) * Vf_ + spreadPos_[v * S4 + l];
                    const float* src = s3 + vs * C3;
                    if (isSilent(src, C3)) continue;
                    addScaled(src, in, C3, inhib3_[vs] ? inh * each : each);
                }
        }

        const float cSpread = sumIn();
        // Upward summary from the voxel's sheet, divisively normalized like the line summary:
        // scaled by 1/sqrt(active sheet cells), so sparse (streamed) and dense (held) input
        // drive the voxel in the same useful range.
        {
            float up[C3] = {};
            uint32_t activeCells = 0;
            for (size_t cell = 0; cell < SS; ++cell) {
                const float* sc = s2 + (v * SS + cell) * C2;
                bool active = false;
                for (uint32_t c = 0; c < C2; ++c) active = active || sc[c] != 0.0f;
                if (!active) continue;
                ++activeCells;
                matvecAdd(U2_.data() + cell * C3 * C2, sc, up, C3, C2, 1.0f);
            }
            if (activeCells > 0) {
                const float norm = cfg_.normalize_upward > 0.0f ? std::sqrt(float(activeCells)) : 1.0f;
                // Not scaled by gain control: a voxel's sheet is driven by the voxel itself, so
                // voxel -> sheet -> voxel is a loop, and amplifying it made activity self-sustain.
                const float scale = gu / norm;
                for (uint32_t c = 0; c < C3; ++c) in[c] += scale * up[c];
            }
        }

        {
            // Gain control turning the field down also turns down its learned recurrent input.
            const float plScale = cfg_.agc_plastic > 0.5f ? std::min(1.0f, afferentGain) : 1.0f;
            const float cUp = sumIn();
            // Learned inhibition from the local interneuron pool (same on every channel).
            const float pool = poolCount ? poolSum / float(poolCount * C3) : 0.0f;
            pool3_[v] = pool;
            float inhibition = 0.0f;
            if (learnedInhibition) {
                inhibition = inhibW3_[v] * pool;
                for (uint32_t c = 0; c < C3; ++c) in[c] -= inhibition;
            }
            float* ds = diagSource3_.data() + v * kSources;
            ds[kFixedLocal] = cLocal;
            ds[kFixedLongRange] = cLong - cLocal;
            ds[kFixed4D] = c4D - cLong;
            ds[kInputDepth] = cDepth - c4D;
            ds[kFixedSpread] = cSpread - cDepth;
            ds[kUpward] = cUp - cSpread;
            ds[kLearnedInhibition] = -inhibition * float(C3);
            ds[kLearnedWithin] = 0.0f;
            ds[kLearned4D] = 0.0f;
            for (uint32_t c = 0; c < C3; ++c) {
                ds[kLearnedWithin] += plScale * pl[c];
                ds[kLearned4D] += pl4[c];
            }
            float plSum = 0.0f, inSum = 0.0f;
            for (uint32_t c = 0; c < C3; ++c) {
                const float learned = plScale * pl[c] + pl4[c];
                in[c] += learned;
                plSum += learned;
                inSum += std::max(0.0f, in[c]);
            }
            diagPlastic3_[v] = plSum;
            diagInput3_[v] = inSum;
        }
        // Field pace: deeper fields integrate their input over more steps (leaky integration).
        if (cfg_.field_pace > 1.0f && f > 0) {
            const float rate = 1.0f / std::pow(cfg_.field_pace, float(f));
            float* u = membrane3_.data() + v * C3;
            for (uint32_t c = 0; c < C3; ++c) {
                u[c] += (in[c] - u[c]) * rate;
                in[c] = u[c];
            }
        }
        drive3_[v] = activateCell<C3>(in, out + v * C3,
                                      theta[v] + (divisiveFatigue ? 0.0f : fatigueShift(lp.fatigue_gain * fatigueMode * fatigue3_[v])),
                                      cfg_.channel_winners3);
    }

    // Pass 2: competition within each field over the inhibition radius, then homeostasis.
    const float* drive = drive3_.data();
    const int R = int(std::min<uint32_t>(cfg_.inhibition_radius3, 3));
#pragma omp parallel for schedule(dynamic, 256)
    for (int64_t vi = 0; vi < voxels; ++vi) {
        const size_t v = size_t(vi);
        const uint32_t x = uint32_t(v % N);
        const uint32_t y = uint32_t((v / N) % N);
        const uint32_t z = uint32_t((v / (size_t(N) * N)) % N);
        const uint32_t f = uint32_t(v / Vf_);
        size_t rivals[342]; // (2*3+1)^3 - 1 at the maximum radius of 3
        uint32_t n = 0;
        for (int dz = -R; dz <= R; ++dz)
            for (int dy = -R; dy <= R; ++dy)
                for (int dx = -R; dx <= R; ++dx) {
                    const int nx = int(x) + dx, ny = int(y) + dy, nz = int(z) + dz;
                    if ((dx == 0 && dy == 0 && dz == 0) || nx < 0 || ny < 0 || nz < 0 || nx >= int(N) ||
                        ny >= int(N) || nz >= int(N))
                        continue;
                    rivals[n++] = voxelIndex(f, uint32_t(nx), uint32_t(ny), uint32_t(nz));
                }
        float final = competeCell<C3>(out + v * C3, drive[v], v, rivals, n, drive, cfg_.winners3, cfg_.output_sigma,
                                         cfg_.fire_threshold3, cfg_.fire_gain3);
        if (divisiveFatigue && final > 0.0f) final = scaleCell<C3>(out + v * C3, 1.0f / (1.0f + lp.fatigue_gain * fatigueMode * fatigue3_[v]));
        adaptThreshold(theta[v], final, lp, target);
        const float firing3 = final * float(C3) / float(std::clamp<uint32_t>(cfg_.channel_winners3, 1, C3));
        fatigue3_[v] += (firing3 - fatigue3_[v]) / std::max(1.0f, lp.fatigue_tau);
        average3_[v] += (final - average3_[v]) / avgTau;
        if (traceTau > 0.0f) {
            float* tr = trace3_.data() + v * C3;
            const float* cell = out + v * C3;
            for (uint32_t c = 0; c < C3; ++c) tr[c] += (cell[c] - tr[c]) / traceTau;
        }
        if (orderTau > 0.0f) {
            float* tr = orderTrace3_.data() + v * C3;
            const float* cell = out + v * C3;
            for (uint32_t c = 0; c < C3; ++c) tr[c] += (cell[c] - tr[c]) / orderTau;
        }
        {
            const float* cell = out + v * C3;
            float strongest = 0.0f, sum = 0.0f;
            for (uint32_t c = 0; c < C3; ++c) {
                strongest = std::max(strongest, cell[c]);
                sum += cell[c];
            }
            const float normMean = strongest > 0.0f ? sum / (strongest * float(C3)) : 0.0f;
            averageN3_[v] += (normMean - averageN3_[v]) / avgTau;
        }
    }

    // Gain control: each field nudges the gain on its incoming signals toward the target
    // share of clearly firing voxels (strongest channel at or above the active level;
    // faint traces do not count). Multiplicative and bounded. With no input the field stays
    // silent whatever its gain, because only incoming signals are scaled.
    // Local gain control: each voxel adjusts its own gain toward the target share of firing
    // voxels in its neighbourhood (radius agc_radius); where the neighbourhood is silent the
    // gain relaxes back toward 1. No field-wide statistic, so a cell sees the same local rule
    // at any size and when the matrix grows.
    // Short-term depression: firing uses transmitter resource, which recovers in time.
    if (depression) {
        const float use = cfg_.learning.depression_use;
        const float recover = 1.0f / std::max(1.0f, cfg_.learning.depression_tau);
#pragma omp parallel for schedule(static)
        for (int64_t i = 0; i < int64_t(V_ * C3); ++i) {
            float& r = resource3_[size_t(i)];
            r = std::clamp(r + (1.0f - r) * recover - use * out[size_t(i)] * r, 0.0f, 1.0f);
        }
    }

    // Inhibitory plasticity: a voxel above its target activity while its neighbourhood is busy
    // strengthens its inhibition, one below it weakens it (Vogels et al. 2011, rate form).
    if (learnedInhibition) {
        const float eta = cfg_.learning.istdp_rate, rho = cfg_.learning.istdp_target;
        const float wMax = std::max(0.0f, cfg_.learning.istdp_max);
        const float tau = cfg_.learning.istdp_tau;
#pragma omp parallel for schedule(static)
        for (int64_t vi = 0; vi < voxels; ++vi) {
            const size_t v = size_t(vi);
            const float pool = pool3_[v];
            float y = 0.0f;
            for (uint32_t c = 0; c < C3; ++c) y += out[v * C3 + c];
            y /= float(C3);
            float signal = pool * (y - rho);
            if (tau > 1.0f) {
                inhibSignal3_[v] += (signal - inhibSignal3_[v]) / tau;
                signal = inhibSignal3_[v];
            } else if (pool <= 0.0f) {
                continue;
            }
            inhibW3_[v] = std::clamp(inhibW3_[v] + eta * signal, 0.0f, wMax);
        }
    }

    if (cfg_.agc_rate > 0.0f && cfg_.agc_local > 0.5f) {
        const float level = lp.active_level;
        std::vector<uint8_t> firing(V_, 0), anything(V_, 0);
#pragma omp parallel for schedule(static)
        for (int64_t vi = 0; vi < voxels; ++vi) {
            const float* cell = out + size_t(vi) * C3;
            float strongest = 0.0f;
            for (uint32_t c = 0; c < C3; ++c) strongest = std::max(strongest, cell[c]);
            firing[size_t(vi)] = strongest >= level;
            anything[size_t(vi)] = strongest > 0.0f;
        }
        const int RA = int(std::min<uint32_t>(cfg_.agc_radius, 4));
        const float relax = std::clamp(cfg_.agc_relax, 0.0f, 1.0f);
#pragma omp parallel for schedule(dynamic, 256)
        for (int64_t vi = 0; vi < voxels; ++vi) {
            const size_t v = size_t(vi);
            const uint32_t x = uint32_t(v % N), y = uint32_t((v / N) % N), z = uint32_t((v / (size_t(N) * N)) % N);
            const uint32_t f = uint32_t(v / Vf_);
            uint32_t count = 0, active = 0;
            bool heard = false;
            for (int dz = -RA; dz <= RA; ++dz)
                for (int dy = -RA; dy <= RA; ++dy)
                    for (int dx = -RA; dx <= RA; ++dx) {
                        const int nx = int(x) + dx, ny = int(y) + dy, nz = int(z) + dz;
                        if (nx < 0 || ny < 0 || nz < 0 || nx >= int(N) || ny >= int(N) || nz >= int(N)) continue;
                        const size_t vn = voxelIndex(f, uint32_t(nx), uint32_t(ny), uint32_t(nz));
                        ++count;
                        active += firing[vn];
                        heard = heard || anything[vn];
                    }
            float& g = voxelGain_[v];
            if (!heard || (cfg_.agc_input_only > 0.5f && !sensoryOn_)) {
                g += (1.0f - g) * relax;
                continue;
            }
            const float share = float(active) / float(std::max<uint32_t>(1, count));
            const float error = (target - share) / std::max(target, 1e-6f);
            g = std::clamp(g * std::exp(cfg_.agc_rate * std::clamp(error, -1.0f, 1.0f)), cfg_.agc_min, cfg_.agc_max);
        }
    } else if (cfg_.agc_rate > 0.0f) {
        const float level = lp.active_level;
        for (uint32_t f = 0; f < kFields; ++f) {
            size_t active = 0;
            bool anything = false;
            for (size_t v = size_t(f) * Vf_; v < size_t(f + 1) * Vf_; ++v) {
                const float* cell = out + v * C3;
                float strongest = 0.0f;
                for (uint32_t c = 0; c < C3; ++c) strongest = std::max(strongest, cell[c]);
                if (strongest > 0.0f) anything = true;
                if (strongest >= level) ++active;
            }
            // Adjust only while there is something to hear. A silent field either holds its gain
            // (agc_relax_field 0; raising it through silence over-amplified the next input) or
            // relaxes it toward 1, so every input starts from the same state (repeatability).
            if (!anything || (cfg_.agc_input_only > 0.5f && !sensoryOn_)) {
                fieldGain_[f] += (1.0f - fieldGain_[f]) * std::clamp(cfg_.agc_relax_field, 0.0f, 1.0f);
                continue;
            }
            const float share = float(active) / float(Vf_);
            const float error = (target - share) / std::max(target, 1e-6f);
            fieldGain_[f] = std::clamp(fieldGain_[f] * std::exp(cfg_.agc_rate * std::clamp(error, -1.0f, 1.0f)),
                                       cfg_.agc_min, cfg_.agc_max);
        }
    }
    s3_.swap();
}

template <class Fn>
void NeuralCellularMatrix::forEachLearnedBlock(size_t v, Fn&& fn) {
    const uint32_t N = N_;
    const uint32_t K = cfg_.long_range_links;
    const uint32_t x = uint32_t(v % N);
    const uint32_t y = uint32_t((v / N) % N);
    const uint32_t z = uint32_t((v / (size_t(N) * N)) % N);
    const uint32_t f = uint32_t(v / Vf_);

    float* w = W3_.data() + v * 27 * C3 * C3;
    for (int dz = -1; dz <= 1; ++dz)
        for (int dy = -1; dy <= 1; ++dy)
            for (int dx = -1; dx <= 1; ++dx) {
                const int nx = int(x) + dx, ny = int(y) + dy, nz = int(z) + dz;
                if (nx < 0 || ny < 0 || nz < 0 || nx >= int(N) || ny >= int(N) || nz >= int(N)) continue;
                const uint32_t o = uint32_t((dz + 1) * 9 + (dy + 1) * 3 + (dx + 1));
                if (o == 13) continue; // self-persistence is not a synapse
                const size_t vn = voxelIndex(f, uint32_t(nx), uint32_t(ny), uint32_t(nz));
                if (!inhib3_[vn]) fn(w + size_t(o) * C3 * C3, vn);
            }
    for (uint32_t l = 0; l < K; ++l) {
        const size_t t = lrTarget_[v * K + l];
        if (!inhib3_[t]) fn(WL_.data() + (v * K + l) * C3 * C3, t);
    }
    uint32_t gi = 0;
    for (uint32_t g = 0; g < kFields; ++g) {
        if (g == f) continue;
        const size_t vg = voxelIndex(g, x, y, z);
        if (!inhib3_[vg]) fn(H_.data() + (v * 3 + gi) * C3 * C3, vg);
        ++gi;
    }
}

void NeuralCellularMatrix::learn(float modulator) {
    const float rate = cfg_.learning.rate * std::clamp(modulator, 0.0f, 1.0f);
    if (rate <= 0.0f) return;
    const float lambda = cfg_.learning.order_gain;
    // After step3D() swapped the buffers, `cur` holds the new state and `next` the previous one.
    const float* post = s3_.cur.data();
    const float* prev = s3_.next.data();
    const float cov = std::clamp(cfg_.learning.covariance, 0.0f, 1.0f);
    const float budget = std::max(0.0f, cfg_.learning.plastic_budget);
    const float oja = std::max(0.0f, cfg_.learning.oja);
    const bool normalized = cfg_.learning.normalized > 0.5f;
    const float soft = std::clamp(cfg_.learning.soft_bound, 0.0f, 1.0f);
    const float predictive = std::clamp(cfg_.learning.predictive, 0.0f, 1.0f);
    const bool useTrace = cfg_.learning.trace_tau > 0.0f;
    const float hetero = std::clamp(cfg_.learning.hetero_ltd, 0.0f, 1.0f);
    const float consolidate = S3_.empty() ? 0.0f : std::clamp(cfg_.learning.consolidation_rate, 0.0f, 1.0f);
    const float slowBudget = std::max(0.0f, cfg_.learning.consolidated_budget);
    // Presynaptic budget: each source channel's total outgoing plastic strength, refreshed
    // every 20 learning steps (it changes slowly). A cell already wired strongly into stored
    // memories forms new outgoing links slowly, so new memories recruit fresh cells instead
    // of the cells they share with old ones (pattern separation).
    const float preSoft = std::clamp(cfg_.learning.presynaptic_bound, 0.0f, 1.0f);
    if (preSoft > 0.0f && (preRoom_.size() != V_ * C3 || learnStats_.calls % 20 == 0)) {
        std::vector<double> out(V_ * C3, 0.0);
        for (size_t t = 0; t < V_; ++t)
            forEachLearnedBlock(t, [&](float* block, size_t src) {
                double* o = out.data() + src * C3;
                for (uint32_t a = 0; a < C3; ++a)
                    for (uint32_t b = 0; b < C3; ++b) o[b] += block[size_t(a) * C3 + b];
            });
        preRoom_.resize(V_ * C3);
        const float outBudget = std::max(1e-6f, cfg_.learning.plastic_budget);
        for (size_t i = 0; i < V_ * C3; ++i) {
            const float usedShare = std::clamp(float(out[i]) / outBudget, 0.0f, 1.0f);
            preRoom_[i] = 1.0f - preSoft * usedShare;
        }
    }
    // Order with a timing window (STDP): "earlier" is each cell's decaying recent activity
    // instead of only the previous step, so j -> i also forms when i starts a few steps
    // after j. The antisymmetric form cancels the shared current step.
    const bool useOrderTrace = cfg_.learning.order_tau > 0.0f;
    auto avgOf = [&](size_t cell) { return cov * (normalized ? averageN3_[cell] : average3_[cell]); };
    // Normalized plasticity: a cell's pattern scaled so its strongest channel is 1.
    auto normalizeInto = [](const float* x, float* outv, uint32_t n) {
        float strongest = 0.0f;
        for (uint32_t c = 0; c < n; ++c) strongest = std::max(strongest, x[c]);
        const float inv = strongest > 0.0f ? 1.0f / strongest : 0.0f;
        for (uint32_t c = 0; c < n; ++c) outv[c] = x[c] * inv;
    };

    const int64_t voxels = int64_t(V_);
    double change = 0.0, scaled = 0.0;
    uint64_t learners = 0;
#pragma omp parallel for schedule(dynamic, 64) reduction(+ : change, scaled, learners)
    for (int64_t vi = 0; vi < voxels; ++vi) {
        const size_t v = size_t(vi);
        const float* pi = post + v * C3;
        const float* qi = useOrderTrace ? orderTrace3_.data() + v * C3 : prev + v * C3;
        if (!anyActive(pi, qi, C3)) continue; // only active cells change their incoming connections
        ++learners;

        float piN[C3], qiN[C3];
        if (normalized) {
            normalizeInto(pi, piN, C3);
            normalizeInto(qi, qiN, C3);
            pi = piN;
            qi = qiN;
        }

        // Soft bounds: each output channel strengthens in proportion to its unused budget.
        float room[C3];
        if (soft > 0.0f) {
            float used[C3] = {};
            forEachLearnedBlock(v, [&](float* block, size_t) {
                for (uint32_t a = 0; a < C3; ++a)
                    for (uint32_t b = 0; b < C3; ++b) used[a] += block[size_t(a) * C3 + b];
            });
            for (uint32_t a = 0; a < C3; ++a) {
                const float freeShare = budget > 0.0f ? std::clamp(1.0f - used[a] / budget, 0.0f, 1.0f) : 0.0f;
                room[a] = 1.0f - soft + soft * freeShare;
            }
        }

        // Local prediction: the drive the cell's plastic inputs gave it from the previous state,
        // at full (recall-mode) strength. Learning stops once memory reproduces the experience.
        float predicted[C3] = {};
        if (predictive > 0.0f) {
            forEachLearnedBlock(v, [&](float* block, size_t src) {
                const float* sq = prev + src * C3;
                if (isSilent(sq, C3)) return;
                matvecAdd(block, sq, predicted, C3, C3, predictive);
            });
        }

        // Inhibitory connections and self-persistence are not visited: they stay fixed.
        forEachLearnedBlock(v, [&](float* block, size_t src) {
            const float* sp = post + src * C3;
            const float* sq = useOrderTrace ? orderTrace3_.data() + src * C3 : prev + src * C3;
            float spN[C3], sqN[C3];
            if (normalized) {
                normalizeInto(sp, spN, C3);
                normalizeInto(sq, sqN, C3);
                sp = spN;
                sq = sqN;
            }
            change += hebbianBlock<C3>(block, pi, qi, sp, sq, avgOf(v), avgOf(src), rate, lambda, oja,
                                        soft > 0.0f ? room : nullptr, predictive > 0.0f ? predicted : nullptr,
                                        useTrace ? trace3_.data() + v * C3 : pi,
                                        useTrace ? trace3_.data() + src * C3 : sp, hetero,
                                        preSoft > 0.0f ? preRoom_.data() + src * C3 : nullptr);
        });

        // Synaptic scaling: cap each output channel's total learned excitatory input.
        // Oja's term alone cannot bound a cell whose output saturates and competes.
        float total[C3] = {};
        auto addRows = [&](const float* block) {
            for (uint32_t a = 0; a < C3; ++a)
                for (uint32_t b = 0; b < C3; ++b) total[a] += block[size_t(a) * C3 + b];
        };
        auto scaleRows = [&](float* block, const float* factor) {
            for (uint32_t a = 0; a < C3; ++a)
                if (factor[a] < 1.0f)
                    for (uint32_t b = 0; b < C3; ++b) block[size_t(a) * C3 + b] *= factor[a];
        };
        forEachLearnedBlock(v, [&](float* block, size_t) { addRows(block); });
        float factor[C3];
        bool scale = false;
        for (uint32_t a = 0; a < C3; ++a) {
            factor[a] = total[a] > budget ? budget / total[a] : 1.0f;
            scale = scale || factor[a] < 1.0f;
            scaled += double(total[a]) * (1.0 - double(factor[a]));
        }
        if (scale) forEachLearnedBlock(v, [&](float* block, size_t) { scaleRows(block, factor); });

        // Consolidation: the slow part follows the fast part upward, then its own budget.
        if (consolidate > 0.0f) {
            float slowTotal[C3] = {};
            forEachLearnedBlock(v, [&](float* block, size_t) {
                float* slow = slowOf(block);
                for (size_t i = 0; i < size_t(C3) * C3; ++i) {
                    if (block[i] > slow[i]) slow[i] += consolidate * (block[i] - slow[i]);
                    slowTotal[i / C3] += slow[i];
                }
            });
            float slowFactor[C3];
            bool slowScale = false;
            for (uint32_t a = 0; a < C3; ++a) {
                slowFactor[a] = slowTotal[a] > slowBudget ? slowBudget / slowTotal[a] : 1.0f;
                slowScale = slowScale || slowFactor[a] < 1.0f;
            }
            if (slowScale) forEachLearnedBlock(v, [&](float* block, size_t) { scaleRows(slowOf(block), slowFactor); });
        }
    }
    learnStats_.calls += 1;
    learnStats_.learners += learners;
    learnStats_.change += change;
    learnStats_.scaled += scaled;

    // Sheet modulation (spec Section 2C): the voxel's C2 x C2 matrix maps each of its
    // sheet cells' previous state to its next input, so it learns from pairs
    // (previous state -> current state), averaged over the sheet, with Oja's bound and
    // the same synaptic-scaling budget as every other plastic connection. (Uncapped, it
    // grew until the sheets sustained their own activity and pulled memories together.)
    // Sheet learning: each active sheet cell associates its state with its summed
    // excitatory neighbourhood (plain Hebbian + order term), capped by its own budget.
    if (!P2_.empty()) {
        const float sheetRate = rate * cfg_.learning.sheet_rate;
        const float sheetBudget = std::max(0.0f, cfg_.learning.sheet_budget);
        const float* s2c = s2_.cur.data();
        const float* s2p = s2_.next.data();
        const uint32_t S = S_;
        const size_t SS = SS_;
        const int64_t cells = int64_t(Q_);
#pragma omp parallel for schedule(dynamic, 256)
        for (int64_t qi = 0; qi < cells; ++qi) {
            const size_t q = size_t(qi);
            const float* post2 = s2c + q * C2;
            const float* pre2 = s2p + q * C2;
            if (!anyActive(post2, pre2, C2)) continue;
            const size_t v = q / SS, cell = q % SS;
            const int sy = int(cell / S), sx = int(cell % S);
            float nCur[C2] = {}, nPrev[C2] = {};
            for (int dy = -1; dy <= 1; ++dy)
                for (int dx = -1; dx <= 1; ++dx) {
                    if (dx == 0 && dy == 0) continue;
                    const int ny = sy + dy, nx = sx + dx;
                    if (ny < 0 || nx < 0 || ny >= int(S) || nx >= int(S)) continue;
                    const size_t qn = v * SS + size_t(ny) * S + size_t(nx);
                    if (inhib2_[qn]) continue;
                    addScaled(s2c + qn * C2, nCur, C2, 1.0f);
                    addScaled(s2p + qn * C2, nPrev, C2, 1.0f);
                }
            float* block = P2_.data() + q * C2 * C2;
            hebbianBlock<C2>(block, post2, pre2, nCur, nPrev, 0.0f, 0.0f, sheetRate, lambda, 0.0f, nullptr, nullptr,
                             post2, nCur, 1.0f, nullptr);
            for (uint32_t a = 0; a < C2; ++a) {
                float* row = block + size_t(a) * C2;
                float total = 0.0f;
                for (uint32_t b = 0; b < C2; ++b) total += row[b];
                if (total > sheetBudget)
                    for (uint32_t b = 0; b < C2; ++b) row[b] *= sheetBudget / total;
            }
        }
    }

    const float modRate = std::max(0.0f, cfg_.learning.modulation_rate);
    if (modRate > 0.0f) {
        const float* s2 = s2_.cur.data();
        const float* s2prev = s2_.next.data();
        const size_t SS = SS_;
        const float sheetRate = rate * modRate / float(SS);
#pragma omp parallel for schedule(dynamic, 64)
        for (int64_t vi = 0; vi < voxels; ++vi) {
            const size_t v = size_t(vi);
            float* M = M2_.data() + v * C2 * C2;
            bool touched = false;
            for (size_t cell = 0; cell < SS; ++cell) {
                const float* post2 = s2 + (v * SS + cell) * C2;
                const float* pre2 = s2prev + (v * SS + cell) * C2;
                float post2N[C2], pre2N[C2];
                if (normalized) {
                    normalizeInto(post2, post2N, C2);
                    normalizeInto(pre2, pre2N, C2);
                    post2 = post2N;
                    pre2 = pre2N;
                }
                for (uint32_t a = 0; a < C2; ++a) {
                    const float pa = post2[a];
                    if (pa == 0.0f) continue;
                    touched = true;
                    float* row = M + size_t(a) * C2;
                    for (uint32_t b = 0; b < C2; ++b)
                        row[b] = std::max(0.0f, row[b] + sheetRate * (pa * pre2[b] - oja * pa * pa * row[b]));
                }
            }
            if (!touched) continue;
            for (uint32_t a = 0; a < C2; ++a) {
                float* row = M + size_t(a) * C2;
                float total = 0.0f;
                for (uint32_t b = 0; b < C2; ++b) total += row[b];
                if (total > budget)
                    for (uint32_t b = 0; b < C2; ++b) row[b] *= budget / total;
            }
        }
    }
}

double NeuralCellularMatrix::plasticFlow(const std::vector<double>& from, const std::vector<double>& to) {
    if (from.size() != V_ * C3 || to.size() != V_ * C3) return 0.0;
    double nf = 0.0, nt = 0.0;
    for (size_t i = 0; i < from.size(); ++i) {
        nf += from[i] * from[i];
        nt += to[i] * to[i];
    }
    if (nf <= 0.0 || nt <= 0.0) return 0.0;
    double flow = 0.0;
    for (size_t v = 0; v < V_; ++v) {
        const double* ti = to.data() + v * C3;
        bool any = false;
        for (uint32_t a = 0; a < C3; ++a) any = any || ti[a] != 0.0;
        if (!any) continue;
        forEachLearnedBlock(v, [&](float* block, size_t src) {
            const double* fj = from.data() + src * C3;
            for (uint32_t a = 0; a < C3; ++a) {
                if (ti[a] == 0.0) continue;
                for (uint32_t b = 0; b < C3; ++b) flow += ti[a] * double(block[size_t(a) * C3 + b]) * fj[b];
            }
        });
    }
    return flow / std::sqrt(nf * nt);
}

double NeuralCellularMatrix::totalPlasticStrength() const {
    double total = 0.0;
    for (const AVec<float>* w : {&W3_, &WL_, &H_})
        for (float x : *w) total += x;
    return total;
}

void NeuralCellularMatrix::clearActivity() {
    for (LevelState* s : {&s1_, &s2_, &s3_}) {
        std::fill(s->cur.begin(), s->cur.end(), 0.0f);
        std::fill(s->next.begin(), s->next.end(), 0.0f);
    }
    std::fill(fatigue2_.begin(), fatigue2_.end(), 0.0f);
    std::fill(fatigue3_.begin(), fatigue3_.end(), 0.0f);
    std::fill(trace3_.begin(), trace3_.end(), 0.0f);
    std::fill(orderTrace3_.begin(), orderTrace3_.end(), 0.0f);
    std::fill(resource3_.begin(), resource3_.end(), 1.0f);
    std::fill(membrane3_.begin(), membrane3_.end(), 0.0f);
    std::fill(lineQuietCur_.begin(), lineQuietCur_.end(), uint8_t(1));
    std::fill(lineQuietNext_.begin(), lineQuietNext_.end(), uint8_t(1));
    std::fill(sheetQuietCur_.begin(), sheetQuietCur_.end(), uint8_t(1));
    std::fill(sheetQuietNext_.begin(), sheetQuietNext_.end(), uint8_t(1));
}

double NeuralCellularMatrix::motorOverlap(const std::vector<uint32_t>& fingerprint) const {
    if (fingerprint.empty()) return 0.0;
    const float level = cfg_.level1.active_level;
    size_t hits = 0;
    for (uint32_t s : fingerprint) {
        if (s >= motorQ_.size()) continue;
        const float* cell = s1_.cur.data() + (size_t(motorQ_[s]) * L_ + (L_ - 1)) * C1;
        float sum = 0.0f;
        for (uint32_t c = 0; c < C1; ++c) sum += cell[c];
        if (sum / float(C1) >= level) ++hits;
    }
    return double(hits) / double(fingerprint.size());
}

MatrixStats NeuralCellularMatrix::computeStats() const {
    MatrixStats st;
    auto levelStats = [](const AVec<float>& state, size_t cellsPerField, uint32_t channels, float level) {
        LevelStats ls;
        for (uint32_t f = 0; f < kFields; ++f) {
            double active = 0.0, total = 0.0;
            const int64_t begin = int64_t(size_t(f) * cellsPerField);
            const int64_t end = begin + int64_t(cellsPerField);
#pragma omp parallel for reduction(+ : active, total) schedule(dynamic, 256)
            for (int64_t i = begin; i < end; ++i) {
                const float* cell = state.data() + size_t(i) * channels;
                float sum = 0.0f, strongest = 0.0f;
                for (uint32_t c = 0; c < channels; ++c) {
                    sum += cell[c];
                    strongest = std::max(strongest, cell[c]);
                }
                total += sum;
                // A cell counts as active when its strongest channel is: with competition
                // inside the cell only a few channels fire, so the mean over all is always low.
                if (strongest >= level) active += 1.0;
            }
            ls.active_fraction[f] = active / double(cellsPerField);
            ls.mean[f] = total / (double(cellsPerField) * channels);
        }
        return ls;
    };
    st.line = levelStats(s1_.cur, P_ / kFields, C1, cfg_.level1.active_level);
    st.sheet = levelStats(s2_.cur, Q_ / kFields, C2, cfg_.level2.active_level);
    st.voxel = levelStats(s3_.cur, V_ / kFields, C3, cfg_.level3.active_level);
    return st;
}

size_t NeuralCellularMatrix::memoryBytes() const {
    auto bytes = [](const auto& vec) { return vec.size() * sizeof(vec[0]); };
    auto level = [&](const LevelState& s) { return bytes(s.cur) + bytes(s.next) + bytes(s.theta); };
    return level(s1_) + level(s2_) + level(s3_) + bytes(inhib2_) + bytes(inhib3_) + bytes(drive2_) +
           bytes(drive3_) + bytes(fatigue2_) + bytes(fatigue3_) + bytes(average3_) + bytes(averageN3_) + bytes(trace3_) + bytes(orderTrace3_) + bytes(W1_) + bytes(W2_) +
           bytes(U1_) + bytes(D1_) + bytes(U2_) + bytes(D2_) + bytes(W3_) + bytes(lrTarget_) + bytes(WL_) +
           bytes(H_) + bytes(M2_) + bytes(sensoryQ_) + bytes(motorQ_) + bytes(sensoryDrive_) + bytes(motorDrive_);
}

} // namespace ncm

namespace ncm {

std::array<DriveBreakdown, kFields> NeuralCellularMatrix::driveBreakdown() const {
    std::array<DriveBreakdown, kFields> out{};
    for (size_t v = 0; v < V_; ++v) {
        DriveBreakdown& d = out[v / Vf_];
        d.plastic += diagPlastic3_[v];
        d.total += diagInput3_[v];
        if (s3_.cur[v * C3] != 0.0f || !isSilent(s3_.cur.data() + v * C3, C3)) {
            d.activeVoxels += 1.0;
            d.plasticActive += diagPlastic3_[v];
            d.totalActive += diagInput3_[v];
        }
    }
    return out;
}

std::array<WeightHealth, kFields> NeuralCellularMatrix::weightHealth() {
    std::array<WeightHealth, kFields> out{};
    const float budget = std::max(1e-6f, cfg_.learning.plastic_budget);
    std::vector<double> outgoing(V_ * C3, 0.0);
    std::array<std::vector<double>, kFields> fills;
    for (size_t v = 0; v < V_; ++v) {
        float used[C3] = {};
        forEachLearnedBlock(v, [&](float* block, size_t src) {
            for (uint32_t a = 0; a < C3; ++a)
                for (uint32_t b = 0; b < C3; ++b) {
                    used[a] += block[size_t(a) * C3 + b];
                    outgoing[src * C3 + b] += block[size_t(a) * C3 + b];
                }
        });
        for (uint32_t a = 0; a < C3; ++a) fills[v / Vf_].push_back(used[a] / budget);
    }
    for (uint32_t f = 0; f < kFields; ++f) {
        WeightHealth& h = out[f];
        const auto& fl = fills[f];
        double sum = 0.0;
        size_t full = 0, used = 0;
        for (double x : fl) {
            sum += x;
            full += x > 0.9;
            used += x > 0.01;
        }
        h.meanFill = fl.empty() ? 0.0 : sum / double(fl.size());
        h.shareFull = fl.empty() ? 0.0 : double(full) / double(fl.size());
        h.shareUsed = fl.empty() ? 0.0 : double(used) / double(fl.size());
        std::vector<double> o(outgoing.begin() + size_t(f) * Vf_ * C3, outgoing.begin() + size_t(f + 1) * Vf_ * C3);
        double total = 0.0;
        for (double x : o) total += x;
        std::sort(o.begin(), o.end(), std::greater<double>());
        double top = 0.0;
        for (size_t i = 0; i < std::max<size_t>(1, o.size() / 100); ++i) top += o[i];
        h.topSourceShare = total > 0.0 ? top / total : 0.0;
    }
    return out;
}

std::array<DriveSources, kFields> NeuralCellularMatrix::driveSources() const {
    std::array<DriveSources, kFields> out{};
    for (size_t v = 0; v < V_; ++v) {
        if (isSilent(s3_.cur.data() + v * C3, C3)) continue;
        DriveSources& d = out[v / Vf_];
        d.firing += 1.0;
        for (uint32_t k = 0; k < kSources; ++k) d.net[k] += diagSource3_[v * kSources + k];
    }
    return out;
}

double NeuralCellularMatrix::meanResource() const {
    double t = 0.0;
    for (float r : resource3_) t += r;
    return resource3_.empty() ? 1.0 : t / double(resource3_.size());
}

std::array<double, kFields> NeuralCellularMatrix::meanInhibitionWeight() const {
    std::array<double, kFields> out{};
    for (size_t v = 0; v < V_; ++v) out[v / Vf_] += inhibW3_[v] / double(Vf_);
    return out;
}

} // namespace ncm
