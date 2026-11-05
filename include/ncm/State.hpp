#pragma once
// Saving and restoring a brain's full state (binary, one file), so one brain can keep learning
// across separate runs: everything that changes while it runs is written; the fixed wiring is
// rebuilt from the seed and checked against a fingerprint.
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <stdexcept>
#include <string>
#include <vector>

namespace ncm {

struct StateFile {
    std::FILE* f = nullptr;
    bool writing = false;
    // Fingerprint mode: nothing is written; every byte that would be saved goes into a 64-bit hash
    // instead (diagnostic counters are left out, so the hash is the brain itself).
    bool hashing = false;
    uint64_t hash = 0x9E3779B97F4A7C15ull;
    struct Fingerprint {};
    explicit StateFile(Fingerprint) : writing(true), hashing(true) {}
    StateFile(const std::string& path, bool write) : writing(write) {
        f = std::fopen(path.c_str(), write ? "wb" : "rb");
        if (!f) throw std::runtime_error("cannot open state file " + path);
    }
    ~StateFile() {
        if (f) std::fclose(f);
    }
    StateFile(const StateFile&) = delete;
    StateFile& operator=(const StateFile&) = delete;

    void bytes(void* p, size_t n) {
        if (hashing) {
            const unsigned char* c = static_cast<const unsigned char*>(p);
            size_t i = 0;
            for (; i + 8 <= n; i += 8) {
                uint64_t w;
                std::memcpy(&w, c + i, 8);
                hash = (hash ^ w) * 0x100000001B3ull;
                hash ^= hash >> 29;
            }
            for (; i < n; ++i) hash = (hash ^ c[i]) * 0x100000001B3ull;
            return;
        }
        const size_t done = writing ? std::fwrite(p, 1, n, f) : std::fread(p, 1, n, f);
        if (done != n) throw std::runtime_error(writing ? "state file: write failed" : "state file: truncated");
    }
    // A plain value (written, or read back into the same variable).
    template <class T>
    void value(T& x) {
        bytes(&x, sizeof(T));
    }
    // A vector: its size, then its elements. Reading checks the size matches (same brain layout).
    template <class V>
    void vec(V& v, const char* what) {
        uint64_t n = v.size();
        value(n);
        if (!writing && n != v.size()) {
            if (!v.empty()) throw std::runtime_error(std::string("state file: size of ") + what + " differs");
            v.resize(n);
        }
        if (n) bytes(v.data(), n * sizeof(v[0]));
    }
    // A list of vectors of doubles (stored patterns of a test).
    void patterns(std::vector<std::vector<double>>& p) {
        uint64_t n = p.size();
        value(n);
        if (!writing) p.assign(n, {});
        for (auto& x : p) vec(x, "pattern");
    }
};

} // namespace ncm
