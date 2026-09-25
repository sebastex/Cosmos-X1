#include "ncm/Matrix.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>

#include "ncm/Random.hpp"

namespace ncm {
namespace {

// y += scale * (A x), with A row-major [rows][cols].
inline void matvecAdd(const float* __restrict A, const float* __restrict x, float* __restrict y,
                      uint32_t rows, uint32_t cols, float scale) {
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

// Bounded, sparse activation f (spec Section 4A): rectified and capped at 1.
inline float activate(float x) { return std::clamp(x, 0.0f, 1.0f); }

// Homeostasis (spec Section 3C) tracks a cell's mean activity, so over time each
// cell averages the target level: strongly on a small share of the time, silent otherwise.
inline void adaptThreshold(float& theta, float meanActivity, const LevelParams& lp, float target) {
    if (lp.homeostasis_rate > 0.0f)
        theta = std::clamp(theta + lp.homeostasis_rate * (meanActivity - target), lp.theta_min, lp.theta_max);
}

// Writes one cell's activated state; returns its drive (mean activated value).
template <uint32_t C>
inline float activateCell(const float* in, float* out, float theta) {
    float sum = 0.0f;
    for (uint32_t c = 0; c < C; ++c) {
        const float v = activate(in[c] - theta);
        out[c] = v;
        sum += v;
    }
    return sum / float(C);
}

// 1D cells: no competition (lines carry sequences intact, spec Section 3C).
template <uint32_t C>
inline void finishCell(const float* in, float* out, float& theta, const LevelParams& lp, float target) {
    adaptThreshold(theta, activateCell<C>(in, out, theta), lp, target);
}

// Lateral inhibition as local competition (spec Section 3C): a cell keeps its
// activity only if its drive is the strongest in its neighbourhood (ties go to
// the lower index); otherwise its neighbours suppress it. Returns the final mean.
template <uint32_t C>
inline float competeCell(float* out, float ownDrive, size_t own, const size_t* rivals, uint32_t rivalCount,
                         const float* drive) {
    bool wins = ownDrive > 0.0f;
    for (uint32_t r = 0; wins && r < rivalCount; ++r) {
        const float d = drive[rivals[r]];
        if (d > ownDrive || (d == ownDrive && rivals[r] < own)) wins = false;
    }
    if (wins) return ownDrive;
    for (uint32_t c = 0; c < C; ++c) out[c] = 0.0f;
    return 0.0f;
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
    const StartingRule& r = cfg_.rule;
    const uint32_t K = cfg_.long_range_links;

    W3_.assign(V_ * 27 * C3 * C3, 0.0f);
    WL_.assign(V_ * K * C3 * C3, 0.0f);
    H_.assign(V_ * 3 * C3 * C3, 0.0f);
    M2_.assign(V_ * C2 * C2, 0.0f);
    lrTarget_.resize(V_ * K);

    const int64_t voxels = int64_t(V_);
#pragma omp parallel for schedule(static)
    for (int64_t vi = 0; vi < voxels; ++vi) {
        const size_t v = size_t(vi);
        Rng rng(mix64(cfg_.seed ^ mix64(kStreamVoxelWeights * 0x100000000ull + v)));

        float* w = W3_.data() + v * 27 * C3 * C3;
        for (size_t i = 0; i < size_t(27) * C3 * C3; ++i) w[i] = float(rng.normal() * r.voxel_noise);
        for (uint32_t o = 0; o < 27; ++o)
            for (uint32_t c = 0; c < C3; ++c)
                w[(size_t(o) * C3 + c) * C3 + c] += (o == 13) ? r.voxel_self : r.voxel_neighbour;

        // Long-range targets: random voxels in the same field, fixed for life (spec Section 3C).
        const size_t fieldBase = (v / Vf_) * Vf_;
        for (uint32_t l = 0; l < K; ++l) {
            size_t t = v;
            uint64_t attempt = 0;
            while (t == v && Vf_ > 1) {
                const double u = hashUniform(cfg_.seed, kStreamLongRange, (uint64_t(v) * K + l) * 64 + attempt++);
                t = fieldBase + std::min(size_t(u * double(Vf_)), Vf_ - 1);
            }
            lrTarget_[v * K + l] = uint32_t(t);
            float* wl = WL_.data() + (v * K + l) * C3 * C3;
            for (size_t i = 0; i < size_t(C3) * C3; ++i) wl[i] = float(rng.normal() * r.voxel_noise);
            setIdentity(wl, C3, r.long_range);
        }

        for (uint32_t g = 0; g < 3; ++g) setIdentity(H_.data() + (v * 3 + g) * C3 * C3, C3, r.link4d);
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
#pragma omp parallel for schedule(static)
    for (int64_t qi = 0; qi < lines; ++qi) {
        const size_t q = size_t(qi);
        const float* parent = s2 + q * C2;
        const float* line = s1 + q * L * C1;
        float* lineOut = out + q * L * C1;
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
    const float* s1 = s1_.cur.data();
    const float* s2 = s2_.cur.data();
    const float* s3 = s3_.cur.data();
    float* out = s2_.next.data();
    float* theta = s2_.theta.data();
    const float gu = cfg_.upward_gain;
    const float gd = cfg_.downward_gain;
    const LevelParams lp = cfg_.level2;
    const float target = cfg_.target_activity;
    const float inh = -cfg_.inhibitory_strength;
    const uint32_t S = S_, L = L_;
    const size_t SS = SS_;

    const int64_t cells = int64_t(Q_);
#pragma omp parallel for schedule(static)
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
                // Self-persistence is not a synapse; lateral inputs carry the sender's sign.
                const float sign = (o == 4 || !inhib2_[qn]) ? 1.0f : inh;
                matvecAdd(W2_.data() + size_t(o) * C2 * C2, s2 + qn * C2, in, C2, C2, sign);
            }
        matvecAdd(M2_.data() + v * C2 * C2, s2 + q * C2, in, C2, C2, 1.0f);
        for (uint32_t k = 0; k < L; ++k)
            matvecAdd(U1_.data() + size_t(k) * C2 * C1, s1 + (q * L + k) * C1, in, C2, C1, gu);
        matvecAdd(D2_.data() + cell * C2 * C3, s3 + v * C3, in, C2, C3, gd);

        drive2_[q] = activateCell<C2>(in, out + q * C2, theta[q]);
    }

    // Pass 2: local competition within each sheet (3 x 3 neighbourhood), then homeostasis.
    const float* drive = drive2_.data();
#pragma omp parallel for schedule(static)
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
        const float final = competeCell<C2>(out + q * C2, drive[q], q, rivals, n, drive);
        adaptThreshold(theta[q], final, lp, target);
    }
    s2_.swap();
}

// 3D level: learned neighbourhood, long-range links, the 4D link to the other
// three fields, and a summary from each voxel's own 2D sheet.
void NeuralCellularMatrix::step3D() {
    const float* s2 = s2_.cur.data();
    const float* s3 = s3_.cur.data();
    float* out = s3_.next.data();
    float* theta = s3_.theta.data();
    const float gu = cfg_.upward_gain;
    const LevelParams lp = cfg_.level3;
    const float target = cfg_.target_activity;
    const float inh = -cfg_.inhibitory_strength;
    const uint32_t N = N_;
    const uint32_t K = cfg_.long_range_links;
    const size_t SS = SS_;

    const int64_t voxels = int64_t(V_);
#pragma omp parallel for schedule(static)
    for (int64_t vi = 0; vi < voxels; ++vi) {
        const size_t v = size_t(vi);
        const uint32_t x = uint32_t(v % N);
        const uint32_t y = uint32_t((v / N) % N);
        const uint32_t z = uint32_t((v / (size_t(N) * N)) % N);
        const uint32_t f = uint32_t(v / Vf_);

        float in[C3] = {};
        const float* w = W3_.data() + v * 27 * C3 * C3;
        for (int dz = -1; dz <= 1; ++dz)
            for (int dy = -1; dy <= 1; ++dy)
                for (int dx = -1; dx <= 1; ++dx) {
                    const int nx = int(x) + dx, ny = int(y) + dy, nz = int(z) + dz;
                    if (nx < 0 || ny < 0 || nz < 0 || nx >= int(N) || ny >= int(N) || nz >= int(N)) continue;
                    const size_t vn = voxelIndex(f, uint32_t(nx), uint32_t(ny), uint32_t(nz));
                    const uint32_t o = uint32_t((dz + 1) * 9 + (dy + 1) * 3 + (dx + 1));
                    const float sign = (o == 13 || !inhib3_[vn]) ? 1.0f : inh;
                    matvecAdd(w + size_t(o) * C3 * C3, s3 + vn * C3, in, C3, C3, sign);
                }

        for (uint32_t l = 0; l < K; ++l) {
            const size_t t = lrTarget_[v * K + l];
            const float sign = inhib3_[t] ? inh : 1.0f;
            matvecAdd(WL_.data() + (v * K + l) * C3 * C3, s3 + t * C3, in, C3, C3, sign);
        }

        uint32_t gi = 0;
        for (uint32_t g = 0; g < kFields; ++g) {
            if (g == f) continue;
            const size_t vg = voxelIndex(g, x, y, z);
            const float sign = inhib3_[vg] ? inh : 1.0f;
            matvecAdd(H_.data() + (v * 3 + gi) * C3 * C3, s3 + vg * C3, in, C3, C3, sign);
            ++gi;
        }

        for (size_t cell = 0; cell < SS; ++cell)
            matvecAdd(U2_.data() + cell * C3 * C2, s2 + (v * SS + cell) * C2, in, C3, C2, gu);

        drive3_[v] = activateCell<C3>(in, out + v * C3, theta[v]);
    }

    // Pass 2: local competition within each field (3 x 3 x 3 neighbourhood), then homeostasis.
    const float* drive = drive3_.data();
#pragma omp parallel for schedule(static)
    for (int64_t vi = 0; vi < voxels; ++vi) {
        const size_t v = size_t(vi);
        const uint32_t x = uint32_t(v % N);
        const uint32_t y = uint32_t((v / N) % N);
        const uint32_t z = uint32_t((v / (size_t(N) * N)) % N);
        const uint32_t f = uint32_t(v / Vf_);
        size_t rivals[26];
        uint32_t n = 0;
        for (int dz = -1; dz <= 1; ++dz)
            for (int dy = -1; dy <= 1; ++dy)
                for (int dx = -1; dx <= 1; ++dx) {
                    const int nx = int(x) + dx, ny = int(y) + dy, nz = int(z) + dz;
                    if ((dx == 0 && dy == 0 && dz == 0) || nx < 0 || ny < 0 || nz < 0 || nx >= int(N) ||
                        ny >= int(N) || nz >= int(N))
                        continue;
                    rivals[n++] = voxelIndex(f, uint32_t(nx), uint32_t(ny), uint32_t(nz));
                }
        const float final = competeCell<C3>(out + v * C3, drive[v], v, rivals, n, drive);
        adaptThreshold(theta[v], final, lp, target);
    }
    s3_.swap();
}

MatrixStats NeuralCellularMatrix::computeStats() const {
    MatrixStats st;
    auto levelStats = [](const AVec<float>& state, size_t cellsPerField, uint32_t channels, float level) {
        LevelStats ls;
        for (uint32_t f = 0; f < kFields; ++f) {
            double active = 0.0, total = 0.0;
            const int64_t begin = int64_t(size_t(f) * cellsPerField);
            const int64_t end = begin + int64_t(cellsPerField);
#pragma omp parallel for reduction(+ : active, total) schedule(static)
            for (int64_t i = begin; i < end; ++i) {
                const float* cell = state.data() + size_t(i) * channels;
                float sum = 0.0f;
                for (uint32_t c = 0; c < channels; ++c) sum += cell[c];
                total += sum;
                if (sum / float(channels) >= level) active += 1.0;
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
           bytes(drive3_) + bytes(W1_) + bytes(W2_) +
           bytes(U1_) + bytes(D1_) + bytes(U2_) + bytes(D2_) + bytes(W3_) + bytes(lrTarget_) + bytes(WL_) +
           bytes(H_) + bytes(M2_) + bytes(sensoryQ_) + bytes(motorQ_) + bytes(sensoryDrive_) + bytes(motorDrive_);
}

} // namespace ncm
