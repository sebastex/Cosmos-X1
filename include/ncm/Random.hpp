#pragma once
#include <cmath>
#include <cstdint>

namespace ncm {

inline uint64_t mix64(uint64_t z) {
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ull;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBull;
    return z ^ (z >> 31);
}

// Deterministic value in [0, 1) for one item: the same seed, stream and index
// always give the same number, so fixed random structure never needs storing twice.
inline double hashUniform(uint64_t seed, uint64_t stream, uint64_t index) {
    const uint64_t h = mix64(seed ^ mix64(stream * 0x9E3779B97F4A7C15ull ^ mix64(index + 0x632BE59BD9B4E019ull)));
    return double(h >> 11) * 0x1.0p-53;
}

// SplitMix64 generator: small, fast and fully reproducible from its seed.
class Rng {
public:
    explicit Rng(uint64_t seed) : state_(seed) {}

    uint64_t next() { return mix64(state_ += 0x9E3779B97F4A7C15ull); }
    double uniform() { return double(next() >> 11) * 0x1.0p-53; }
    uint64_t below(uint64_t n) { return next() % n; }

    double normal() {
        double u1 = uniform();
        if (u1 < 1e-300) u1 = 1e-300;
        const double u2 = uniform();
        return std::sqrt(-2.0 * std::log(u1)) * std::cos(6.283185307179586 * u2);
    }

private:
    uint64_t state_;
};

// Streams keep different kinds of fixed random structure independent of each other.
enum Stream : uint64_t {
    kStreamInhibitory2 = 1,
    kStreamInhibitory3,
    kStreamPositionUp1,
    kStreamPositionDown1,
    kStreamPositionUp2,
    kStreamPositionDown2,
    kStreamVoxelWeights,
    kStreamLongRange,
    kStreamCodebook,
};

} // namespace ncm
