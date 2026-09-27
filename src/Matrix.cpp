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

// Lateral inhibition as local competition (spec Section 3C): a cell keeps its
// activity only if fewer than `winners` of its neighbours are driven harder
// (ties go to the lower index); otherwise its neighbours suppress it. Allowing a
// few winners per neighbourhood lets small groups of neighbouring cells fire
// together, which Hebbian learning needs to bind them. Returns the final mean.
template <uint32_t C>
inline float competeCell(float* out, float ownDrive, size_t own, const size_t* rivals, uint32_t rivalCount,
                         const float* drive, uint32_t winners, float sigma = 0.0f) {
    uint32_t stronger = 0;
    bool wins = ownDrive > 0.0f;
    for (uint32_t r = 0; wins && r < rivalCount; ++r) {
        const float d = drive[rivals[r]];
        if (d > ownDrive || (d == ownDrive && rivals[r] < own))
            if (++stronger >= winners) wins = false;
    }
    if (wins) {
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
                           float oja, const float* room, const float* predicted) {
    double change = 0.0;
    for (uint32_t a = 0; a < C; ++a) {
        const float pa = post[a], qa = postPrev[a];
        if (pa == 0.0f && qa == 0.0f) continue;
        float* row = W + size_t(a) * C;
        const float da = pa - avgPost - (predicted ? predicted[a] : 0.0f);
        const float up = room ? room[a] : 1.0f;
        for (uint32_t b = 0; b < C; ++b) {
            const float dw =
                da * (pre[b] - avgPre) + lambda * (prePrev[b] * pa - pre[b] * qa) - oja * pa * pa * row[b];
            const float updated = std::max(0.0f, row[b] + rate * (dw > 0.0f ? up * dw : dw));
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
    s2_.allocate(Q_, C2);
    s3_.allocate(V_, C3);
    drive2_.assign(Q_, 0.0f);
    drive3_.assign(V_, 0.0f);
    fatigue2_.assign(Q_, 0.0f);
    fatigue3_.assign(V_, 0.0f);
    average3_.assign(V_, cfg.target_activity);
    averageN3_.assign(V_, cfg.target_activity);

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
        }
    }
    if (motorOn_) {
        for (size_t s = 0; s < motorQ_.size(); ++s) {
            float* exitCell = s1_.next.data() + (size_t(motorQ_[s]) * L_ + (L_ - 1)) * C1;
            const float v = motorDrive_[s] ? 1.0f : 0.0f;
            for (uint32_t c = 0; c < C1; ++c) exitCell[c] = v;
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
        // A silent line under a silent parent stays silent: skip the arithmetic.
        if (isSilent(parent, C2) && isSilent(line, size_t(L) * C1)) {
            std::fill(lineOut, lineOut + size_t(L) * C1, 0.0f);
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
    }

    applySurfaceClamps();
    s1_.swap();
}

// 2D level: lateral interaction within each sheet, a summary from each cell's own
// 1D line, drive from the parent voxel, and the voxel's learned modulation.
void NeuralCellularMatrix::step2D() {
    // Fatigue at full strength while encoding and in silence (it ends activity that outlasts
    // its input); in recall mode (M = 0) it is scaled to fatigue_recall so a recalled memory
    // can settle instead of wearing itself out.
    const float fatigueMode = std::clamp(cfg_.fatigue_recall, 0.0f, 1.0f) +
                              (1.0f - std::clamp(cfg_.fatigue_recall, 0.0f, 1.0f)) * std::clamp(modulator_, 0.0f, 1.0f);
    const float* s1 = s1_.cur.data();
    const float* s2 = s2_.cur.data();
    const float* s3 = s3_.cur.data();
    float* out = s2_.next.data();
    float* theta = s2_.theta.data();
    const float gd = cfg_.downward_gain;
    const LevelParams lp = cfg_.level2;
    const float target = cfg_.target_activity;
    const float inh = -cfg_.inhibitory_strength;
    const uint32_t S = S_, L = L_;
    const size_t SS = SS_;

    const int64_t cells = int64_t(Q_);
#pragma omp parallel for schedule(dynamic, 256)
    for (int64_t qi = 0; qi < cells; ++qi) {
        const size_t q = size_t(qi);
        const size_t v = q / SS;
        const size_t cell = q % SS;
        const int sy = int(cell / S), sx = int(cell % S);

        float in[C2] = {};
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
            }
        if (!isSilent(s2 + q * C2, C2)) matvecAdd(M2_.data() + v * C2 * C2, s2 + q * C2, in, C2, C2, 1.0f);
        // Upward summary with divisive normalization: scaled by 1/sqrt(active line cells), so
        // a streamed character (one active cell per line) and a held one (a full line) drive
        // the sheet cell in the same useful range.
        {
            float up[C2] = {};
            uint32_t activeCells = 0;
            for (uint32_t k = 0; k < L; ++k) {
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

        drive2_[q] = activateCell<C2>(in, out + q * C2, theta[q] + lp.fatigue_gain * fatigueMode * fatigue2_[q],
                                      cfg_.channel_winners2);
    }

    // Pass 2: local competition within each sheet (3 x 3 neighbourhood), then homeostasis.
    const float* drive = drive2_.data();
#pragma omp parallel for schedule(dynamic, 256)
    for (int64_t qi = 0; qi < cells; ++qi) {
        const size_t q = size_t(qi);
        const size_t v = q / SS;
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
        const float final = competeCell<C2>(out + q * C2, drive[q], q, rivals, n, drive, cfg_.winners2, cfg_.output_sigma);
        adaptThreshold(theta[q], final, lp, target);
        // Fatigue follows the activity of the channels that fire (the mean over all channels
        // understates it by C / channel_winners, so fatigue could never build up).
        const float firing2 = final * float(C2) / float(std::clamp<uint32_t>(cfg_.channel_winners2, 1, C2));
        fatigue2_[q] += (firing2 - fatigue2_[q]) / std::max(1.0f, lp.fatigue_tau);
    }
    s2_.swap();
}

// 3D level: learned neighbourhood, long-range links, the 4D link to the other
// three fields, and a summary from each voxel's own 2D sheet.
void NeuralCellularMatrix::step3D() {
    // Fatigue at full strength while encoding and in silence (it ends activity that outlasts
    // its input); in recall mode (M = 0) it is scaled to fatigue_recall so a recalled memory
    // can settle instead of wearing itself out.
    const float fatigueMode = std::clamp(cfg_.fatigue_recall, 0.0f, 1.0f) +
                              (1.0f - std::clamp(cfg_.fatigue_recall, 0.0f, 1.0f)) * std::clamp(modulator_, 0.0f, 1.0f);
    const float* s2 = s2_.cur.data();
    const float* s3 = s3_.cur.data();
    float* out = s3_.next.data();
    float* theta = s3_.theta.data();
    const float gu = cfg_.upward_gain;
    const LevelParams lp = cfg_.level3;
    const float target = cfg_.target_activity;
    const float inh = -cfg_.inhibitory_strength;
    const float avgTau = std::max(1.0f, cfg_.learning.average_tau);
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

    const int64_t voxels = int64_t(V_);
#pragma omp parallel for schedule(dynamic, 256)
    for (int64_t vi = 0; vi < voxels; ++vi) {
        const size_t v = size_t(vi);
        const uint32_t x = uint32_t(v % N);
        const uint32_t y = uint32_t((v / N) % N);
        const uint32_t z = uint32_t((v / (size_t(N) * N)) % N);
        const uint32_t f = uint32_t(v / Vf_);

        float in[C3] = {};
        const float afferentGain = fieldGain_[f]; // gain control scales incoming signals only
        const float* w = W3_.data() + v * 27 * C3 * C3;
        for (int dz = -1; dz <= 1; ++dz)
            for (int dy = -1; dy <= 1; ++dy)
                for (int dx = -1; dx <= 1; ++dx) {
                    const int nx = int(x) + dx, ny = int(y) + dy, nz = int(z) + dz;
                    if (nx < 0 || ny < 0 || nz < 0 || nx >= int(N) || ny >= int(N) || nz >= int(N)) continue;
                    const size_t vn = voxelIndex(f, uint32_t(nx), uint32_t(ny), uint32_t(nz));
                    const uint32_t o = uint32_t((dz + 1) * 9 + (dy + 1) * 3 + (dx + 1));
                    const float* src = s3 + vn * C3;
                    if (isSilent(src, C3)) continue; // silent sources contribute nothing
                    if (o == 13) {
                        addScaled(src, in, C3, r.voxel_self); // self-persistence: fixed
                    } else if (inhib3_[vn]) {
                        addScaled(src, in, C3, inh * r.voxel_neighbour); // inhibitory: fixed scaffold only
                    } else {
                        addScaled(src, in, C3, r.voxel_neighbour);                // scaffold
                        matvecAdd(w + size_t(o) * C3 * C3, src, in, C3, C3, rec); // plastic memory part
                    }
                }

        for (uint32_t l = 0; l < K; ++l) {
            const size_t t = lrTarget_[v * K + l];
            const float* src = s3 + t * C3;
            if (isSilent(src, C3)) continue;
            if (inhib3_[t]) {
                addScaled(src, in, C3, inh * r.long_range);
            } else {
                addScaled(src, in, C3, r.long_range);
                matvecAdd(WL_.data() + (v * K + l) * C3 * C3, src, in, C3, C3, rec);
            }
        }

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
            const float link = g < f ? r.link4d : r.link4d_backward; // feedforward vs feedback
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
                matvecAdd(H_.data() + (v * 3 + gi) * C3 * C3, src, in, C3, C3, gain * plasticScale * rec4);
            }
            ++gi;
        }

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

        drive3_[v] = activateCell<C3>(in, out + v * C3, theta[v] + lp.fatigue_gain * fatigueMode * fatigue3_[v],
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
        const float final = competeCell<C3>(out + v * C3, drive[v], v, rivals, n, drive, cfg_.winners3, cfg_.output_sigma);
        adaptThreshold(theta[v], final, lp, target);
        const float firing3 = final * float(C3) / float(std::clamp<uint32_t>(cfg_.channel_winners3, 1, C3));
        fatigue3_[v] += (firing3 - fatigue3_[v]) / std::max(1.0f, lp.fatigue_tau);
        average3_[v] += (final - average3_[v]) / avgTau;
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
    if (cfg_.agc_rate > 0.0f) {
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
            // Adjust only while there is something to hear: a silent field holds its gain.
            // (Raising it through silence over-amplified the next input and made responses
            // depend on how long the silence lasted.)
            if (!anything) continue;
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
        const float* qi = prev + v * C3;
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
            const float* sq = prev + src * C3;
            float spN[C3], sqN[C3];
            if (normalized) {
                normalizeInto(sp, spN, C3);
                normalizeInto(sq, sqN, C3);
                sp = spN;
                sq = sqN;
            }
            change += hebbianBlock<C3>(block, pi, qi, sp, sq, avgOf(v), avgOf(src), rate, lambda, oja,
                                        soft > 0.0f ? room : nullptr, predictive > 0.0f ? predicted : nullptr);
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
           bytes(drive3_) + bytes(fatigue2_) + bytes(fatigue3_) + bytes(average3_) + bytes(averageN3_) + bytes(W1_) + bytes(W2_) +
           bytes(U1_) + bytes(D1_) + bytes(U2_) + bytes(D2_) + bytes(W3_) + bytes(lrTarget_) + bytes(WL_) +
           bytes(H_) + bytes(M2_) + bytes(sensoryQ_) + bytes(motorQ_) + bytes(sensoryDrive_) + bytes(motorDrive_);
}

} // namespace ncm
