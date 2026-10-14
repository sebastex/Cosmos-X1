#include "ncm/Monitor.hpp"

#include <algorithm>
#include <atomic>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <stdexcept>

#include "ncm/Matrix.hpp"

namespace ncm {

namespace {

uint64_t envNumber(const char* name, uint64_t fallback) {
    const char* s = std::getenv(name);
    return s && *s ? std::strtoull(s, nullptr, 10) : fallback;
}

template <class T>
void put(std::FILE* f, const T& x) {
    std::fwrite(&x, sizeof(T), 1, f);
}

} // namespace

std::unique_ptr<Monitor> Monitor::fromEnv(const Config& cfg) {
    const char* dir = std::getenv("NCM_MONITOR");
    if (!dir || !*dir) return nullptr;
    static std::atomic<uint32_t> sessions{0};
    std::filesystem::create_directories(dir);
    const std::string file = std::string(dir) + "/session" + std::to_string(sessions++) + ".ncmmon";
    const char* vox = std::getenv("NCM_MONITOR_VOXELS");
    return std::make_unique<Monitor>(cfg, file, std::max<uint64_t>(1, envNumber("NCM_MONITOR_EVERY", 1)),
                                     envNumber("NCM_MONITOR_FROM", 0), envNumber("NCM_MONITOR_TO", UINT64_MAX),
                                     vox && std::atoi(vox) > 0);
}

Monitor::Monitor(const Config& cfg, const std::string& file, uint64_t every, uint64_t from, uint64_t to, bool voxels)
    : every_(every), from_(from), to_(to), voxels_(voxels) {
    f_ = std::fopen(file.c_str(), "wb");
    if (!f_) throw std::runtime_error("monitor: cannot write " + file);
    N_ = cfg.field_dim;
    SS_ = uint32_t(cfg.sheetCellsPerVoxel());
    L_ = cfg.line_len;
    V_ = cfg.voxels();
    Vf_ = cfg.voxelsPerField();
    // 4D cells: 4 x 4 x 4 blocks per w-slice when the side divides by 4, else one group per slice.
    A_ = N_ % 4 == 0 ? 4 : 1;
    B_ = N_ / A_;
    G_ = A_ * A_ * A_ * kFields;
    level1_ = cfg.level1.active_level;
    level2_ = cfg.level2.active_level;
    level3_ = cfg.level3.active_level;
    groupOf_.resize(V_);
    for (size_t v = 0; v < V_; ++v) {
        const uint32_t x = uint32_t(v % N_), y = uint32_t((v / N_) % N_), z = uint32_t((v / (size_t(N_) * N_)) % N_);
        const uint32_t w = uint32_t(v / Vf_);
        groupOf_[v] = ((w * A_ + z / B_) * A_ + y / B_) * A_ + x / B_;
    }
    for (auto* a : {&a1_, &a2_, &a3_}) a->assign(V_, 0);
    for (auto* s : {&s1_, &s2_, &s3_, &d4_}) s->assign(V_, 0.0f);

    // Header (little-endian): magic, version, geometry, levels, options.
    std::fwrite("NCMMON01", 1, 8, f_);
    const uint32_t head[] = {1u, N_, SS_, L_, C1, C2, C3, kFields, A_, B_, G_, cfg.lattice4d > 0.5f ? 1u : 0u,
                             voxels_ ? 1u : 0u, uint32_t(std::min<uint64_t>(every_, UINT32_MAX))};
    std::fwrite(head, sizeof(uint32_t), sizeof(head) / sizeof(uint32_t), f_);
    const float levels[] = {level1_, level2_, level3_};
    std::fwrite(levels, sizeof(float), 3, f_);
    put(f_, uint64_t(V_));
    put(f_, uint64_t(cfg.sheetCells()));
    put(f_, uint64_t(cfg.lineCells()));
}

Monitor::~Monitor() {
    if (f_) std::fclose(f_);
}

void Monitor::record(const NeuralCellularMatrix& m, const Event& e) {
    if (e.tick < from_ || e.tick > to_ || e.tick % every_ != 0) return;
    const float* line = m.lineState().data();
    const float* sheet = m.sheetState().data();
    const float* vox = m.voxelState().data();
    const std::vector<uint8_t>& lineQuiet = m.lineQuiet();
    const std::vector<uint8_t>& sheetQuiet = m.sheetQuiet();
    const AVec<float>& drive4f = m.latticeDriveFixed();
    const AVec<float>& drive4l = m.latticeDriveLearned();
    const size_t SS = SS_, L = L_;

    const int64_t voxels = int64_t(V_);
#pragma omp parallel for schedule(dynamic, 256)
    for (int64_t vi = 0; vi < voxels; ++vi) {
        const size_t v = size_t(vi);
        auto cellStats = [](const float* c, uint32_t n, float level, uint32_t& active, float& sum) {
            float strongest = 0.0f, total = 0.0f;
            for (uint32_t k = 0; k < n; ++k) {
                strongest = std::max(strongest, c[k]);
                total += c[k];
            }
            sum += total;
            if (strongest >= level && strongest > 0.0f) ++active;
        };
        uint32_t a1 = 0, a2 = 0, a3 = 0;
        float s1 = 0.0f, s2 = 0.0f, s3 = 0.0f;
        cellStats(vox + v * C3, C3, level3_, a3, s3);
        for (size_t cell = 0; cell < SS; ++cell) {
            const size_t q = v * SS + cell;
            if (!sheetQuiet[v]) cellStats(sheet + q * C2, C2, level2_, a2, s2);
            if (!lineQuiet[q])
                for (size_t k = 0; k < L; ++k) cellStats(line + (q * L + k) * C1, C1, level1_, a1, s1);
        }
        float d = 0.0f;
        if (!drive4f.empty())
            for (uint32_t c = 0; c < C3; ++c) d += drive4f[v * C3 + c] + drive4l[v * C3 + c];
        a1_[v] = a1, a2_[v] = a2, a3_[v] = a3;
        s1_[v] = s1, s2_[v] = s2, s3_[v] = s3, d4_[v] = d;
    }

    // Per 4D cell: active cells and summed activity at each level, and its 4D-lattice drive.
    std::vector<uint32_t> ga1(G_, 0), ga2(G_, 0), ga3(G_, 0);
    std::vector<float> gs1(G_, 0.0f), gs2(G_, 0.0f), gs3(G_, 0.0f), gd4(G_, 0.0f);
    uint32_t t1 = 0, t2 = 0, t3 = 0;
    float u1 = 0.0f, u2 = 0.0f, u3 = 0.0f;
    for (size_t v = 0; v < V_; ++v) {
        const uint32_t g = groupOf_[v];
        ga1[g] += a1_[v], ga2[g] += a2_[v], ga3[g] += a3_[v];
        gs1[g] += s1_[v], gs2[g] += s2_[v], gs3[g] += s3_[v], gd4[g] += d4_[v];
        t1 += a1_[v], t2 += a2_[v], t3 += a3_[v];
        u1 += s1_[v], u2 += s2_[v], u3 += s3_[v];
    }

    // Record: tick, what happened, totals per level, then every 4D cell.
    put(f_, e.tick);
    const uint8_t flags = uint8_t((e.sheet ? 1 : 0) | (e.voxel ? 2 : 0) | (e.fourD ? 4 : 0) | (e.sensory ? 8 : 0) |
                                  (e.learning ? 16 : 0));
    put(f_, flags);
    put(f_, uint8_t(e.heard));
    put(f_, uint16_t(0));
    put(f_, e.modulator);
    put(f_, t1), put(f_, u1), put(f_, t2), put(f_, u2), put(f_, t3), put(f_, u3);
    for (uint32_t g = 0; g < G_; ++g) {
        put(f_, ga1[g]), put(f_, gs1[g]), put(f_, ga2[g]), put(f_, gs2[g]), put(f_, ga3[g]), put(f_, gs3[g]);
        put(f_, gd4[g]);
    }
    if (voxels_) {
        // Every voxel with any activity: index and its channels (0-255).
        uint32_t n = 0;
        for (size_t v = 0; v < V_; ++v) n += s3_[v] > 0.0f;
        put(f_, n);
        for (size_t v = 0; v < V_; ++v) {
            if (s3_[v] <= 0.0f) continue;
            put(f_, uint32_t(v));
            uint8_t ch[C3];
            for (uint32_t c = 0; c < C3; ++c) ch[c] = uint8_t(std::clamp(vox[v * C3 + c], 0.0f, 1.0f) * 255.0f + 0.5f);
            std::fwrite(ch, 1, C3, f_);
        }
    }
}

} // namespace ncm
