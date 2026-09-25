#include "ncm/Viewer.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <stdexcept>
#include <vector>

namespace ncm {
namespace {

struct Image {
    int width, height;
    std::vector<uint8_t> rgb; // top-down, 3 bytes per pixel

    Image(int w, int h, uint8_t background) : width(w), height(h), rgb(size_t(w) * h * 3, background) {}

    void set(int x, int y, uint8_t r, uint8_t g, uint8_t b) {
        uint8_t* p = rgb.data() + (size_t(y) * width + x) * 3;
        p[0] = r;
        p[1] = g;
        p[2] = b;
    }
};

// Heat colour map, black -> red -> yellow -> white. The square root lifts
// sparse, faint activity so it stays visible.
void heat(float v, uint8_t& r, uint8_t& g, uint8_t& b) {
    const float t = std::sqrt(std::clamp(v, 0.0f, 1.0f));
    r = uint8_t(255.0f * std::clamp(3.0f * t, 0.0f, 1.0f));
    g = uint8_t(255.0f * std::clamp(3.0f * t - 1.0f, 0.0f, 1.0f));
    b = uint8_t(255.0f * std::clamp(3.0f * t - 2.0f, 0.0f, 1.0f));
}

void writeBmp(const Image& img, const std::string& path) {
    const int rowBytes = (img.width * 3 + 3) & ~3;
    const uint32_t dataSize = uint32_t(rowBytes) * uint32_t(img.height);
    const uint32_t fileSize = 54 + dataSize;

    std::vector<uint8_t> header(54, 0);
    auto put16 = [&](size_t at, uint16_t v) {
        header[at] = uint8_t(v);
        header[at + 1] = uint8_t(v >> 8);
    };
    auto put32 = [&](size_t at, uint32_t v) {
        for (int i = 0; i < 4; ++i) header[at + i] = uint8_t(v >> (8 * i));
    };
    header[0] = 'B';
    header[1] = 'M';
    put32(2, fileSize);
    put32(10, 54);
    put32(14, 40);
    put32(18, uint32_t(img.width));
    put32(22, uint32_t(img.height));
    put16(26, 1);
    put16(28, 24);
    put32(34, dataSize);

    std::ofstream out(path, std::ios::binary);
    if (!out) throw std::runtime_error("cannot write " + path);
    out.write(reinterpret_cast<const char*>(header.data()), std::streamsize(header.size()));

    std::vector<uint8_t> row(size_t(rowBytes), 0);
    for (int y = img.height - 1; y >= 0; --y) { // BMP rows run bottom-up
        for (int x = 0; x < img.width; ++x) {
            const uint8_t* p = img.rgb.data() + (size_t(y) * img.width + x) * 3;
            row[size_t(x) * 3 + 0] = p[2]; // BMP stores BGR
            row[size_t(x) * 3 + 1] = p[1];
            row[size_t(x) * 3 + 2] = p[0];
        }
        out.write(reinterpret_cast<const char*>(row.data()), rowBytes);
    }
}

float meanOf(const float* cell, uint32_t channels) {
    float sum = 0.0f;
    for (uint32_t c = 0; c < channels; ++c) sum += cell[c];
    return sum / float(channels);
}

} // namespace

void writeSnapshot(const NeuralCellularMatrix& m, const std::string& path, int scale) {
    const Config& cfg = m.config();
    const int N = int(cfg.field_dim), S = int(cfg.sheet_dim), L = int(cfg.line_len);
    const size_t SS = cfg.sheetCellsPerVoxel();
    const int panel = N * S * scale;
    const int gap = 6;
    Image img(int(kFields) * panel + (int(kFields) + 1) * gap, 3 * panel + 4 * gap, 40);

    const auto& s1 = m.lineState();
    const auto& s2 = m.sheetState();
    const auto& s3 = m.voxelState();
    const uint32_t z = uint32_t(N / 2);

    for (uint32_t f = 0; f < kFields; ++f) {
        const int ox = gap + int(f) * (panel + gap);
        for (int py = 0; py < N * S; ++py)
            for (int px = 0; px < N * S; ++px) {
                const uint32_t x = uint32_t(px / S), y = uint32_t(py / S);
                const size_t v = m.voxelIndex(f, x, y, z);
                const size_t q = v * SS + size_t(py % S) * S + size_t(px % S);

                const float values[3] = {
                    meanOf(s3.data() + v * C3, C3),
                    meanOf(s2.data() + q * C2, C2),
                    meanOf(s1.data() + q * L * C1, uint32_t(L) * C1),
                };
                for (int row = 0; row < 3; ++row) {
                    uint8_t r, g, b;
                    heat(values[row], r, g, b);
                    const int oy = gap + row * (panel + gap);
                    for (int dy = 0; dy < scale; ++dy)
                        for (int dx = 0; dx < scale; ++dx)
                            img.set(ox + px * scale + dx, oy + py * scale + dy, r, g, b);
                }
            }
    }
    writeBmp(img, path);
}

} // namespace ncm
