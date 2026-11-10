#pragma once
// Learned links stored link by link: only the links that hold something take memory.
//
// Every learned connection between two voxels is a C3 x C3 block of plastic weights (and, with
// consolidation, a slow block of the same size). A link that is exactly zero adds nothing to any
// sum, so it need not be stored. Each voxel keeps all of its incoming blocks (near, long-range,
// 4D, in that order) in one record:
//   - an empty block takes no room;
//   - a block with few links keeps only those links: where each sits (one byte) and its value
//     (fast, and slow beside it);
//   - a block with many links is kept whole (positions would cost more than the zeros).
// Sums over a block keep their order (row by row, column by column), and a skipped link only ever
// added a zero, so the brain computes exactly what it computed with every block stored whole
// (proved by the brain fingerprint, NCM_HASH). Saved brain files keep the full (dense) layout.
//
// Learning works on a voxel's blocks unpacked (open): one buffer per thread, all of the voxel's
// blocks whole, each fast block followed by its slow block. Closing packs them back.
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <string>
#include <vector>

#include "ncm/Buffer.hpp"
#include "ncm/Config.hpp"
#include "ncm/State.hpp"

namespace ncm {

class LinkStore {
public:
    static constexpr size_t kBlock = size_t(C3) * C3; // links in one block
    static constexpr uint32_t kWhole = 224;           // a block with more links than this is kept whole

    // One block as stored: n links (0 = empty); pos = where each sits (row * C3 + column, in
    // order), or nullptr when the block is kept whole (n = kBlock); slow = nullptr without
    // consolidation.
    struct View {
        uint32_t n = 0;
        const uint8_t* pos = nullptr;
        const float* fast = nullptr;
        const float* slow = nullptr;
    };

    LinkStore() = default;
    LinkStore(const LinkStore&) = delete;
    LinkStore& operator=(const LinkStore&) = delete;
    ~LinkStore() {
        Work& w = work();
        if (w.owner == this) w.owner = nullptr; // the buffer is zero again only after a close; drop it
        if (w.owner == nullptr && w.dirty) w.buf.clear(), w.dirty = false;
    }

    void init(size_t voxels, uint32_t blocksPerVoxel, bool withSlow) {
        if (blocksPerVoxel == 0) throw std::runtime_error("LinkStore: no blocks");
        V_ = voxels;
        nb_ = blocksPerVoxel;
        slow_ = withSlow;
        rec_.clear();
        rec_.resize(voxels);
    }
    bool hasSlow() const { return slow_; }
    uint32_t blocksPerVoxel() const { return nb_; }
    size_t voxels() const { return V_; }

    View view(size_t v, uint32_t b) const {
        const Rec& r = rec_[v];
        if (r.off.empty()) return {};
        const uint32_t o = r.off[b], n = r.off[b + 1] - o;
        if (!n) return {};
        const uint32_t p = r.poff[b];
        return {n, r.poff[b + 1] != p ? r.pos.data() + p : nullptr, r.fast.data() + o,
                slow_ ? r.slow.data() + o : nullptr};
    }

    // Bytes held (what the records reserve, plus the table of records).
    size_t bytes() const {
        size_t s = rec_.capacity() * sizeof(Rec);
        for (const Rec& r : rec_)
            s += (r.off.capacity() + r.poff.capacity()) * sizeof(uint32_t) + r.pos.capacity() +
                 (r.fast.capacity() + r.slow.capacity()) * sizeof(float);
        return s;
    }

    // --- The workspace: voxel v's blocks unpacked (fast block b at b * 2 * kBlock, its slow block
    // right after it). Only one voxel per thread is open at a time.
    bool isOpen(size_t v) const {
        const Work& w = work();
        return w.owner == this && w.v == v;
    }
    float* open(size_t v) {
        Work& w = work();
        if (w.owner == this && w.v == v) return w.buf.data();
        if (w.owner) w.owner->close(true);
        const size_t need = size_t(nb_) * 2 * kBlock;
        if (w.buf.size() < need) w.buf.assign(need, 0.0f); // zero between uses
        float* buf = w.buf.data();
        for (uint32_t b = 0; b < nb_; ++b) {
            const View x = view(v, b);
            if (!x.n) continue;
            float* f = buf + size_t(b) * 2 * kBlock;
            if (!x.pos) {
                std::memcpy(f, x.fast, kBlock * sizeof(float));
                if (x.slow) std::memcpy(f + kBlock, x.slow, kBlock * sizeof(float));
            } else {
                for (uint32_t k = 0; k < x.n; ++k) f[x.pos[k]] = x.fast[k];
                if (x.slow)
                    for (uint32_t k = 0; k < x.n; ++k) f[kBlock + x.pos[k]] = x.slow[k];
            }
        }
        w.owner = this;
        w.v = v;
        w.dirty = true;
        return buf;
    }
    // Closes the workspace; keep = pack the blocks back into the voxel's record (else they were
    // only read, and the record stays as it is).
    void close(bool keep) {
        Work& w = work();
        if (w.owner != this) return;
        float* buf = w.buf.data();
        const size_t v = w.v;
        w.owner = nullptr;
        w.v = SIZE_MAX;
        if (!keep) {
            for (uint32_t b = 0; b < nb_; ++b)
                if (view(v, b).n) std::memset(buf + size_t(b) * 2 * kBlock, 0, 2 * kBlock * sizeof(float));
            w.dirty = false;
            return;
        }
        thread_local Rec t;
        pack(buf, t);
        w.dirty = false;
        Rec& r = rec_[v];
        if (t.fast.empty()) {
            r = Rec();
            return;
        }
        copyInto(r.off, t.off);
        copyInto(r.poff, t.poff);
        copyInto(r.pos, t.pos);
        copyInto(r.fast, t.fast);
        copyInto(r.slow, t.slow);
    }
    // Keeps one voxel open across several passes (the learning step): while a Hold is alive on
    // this thread, passes leave the workspace open; it is packed when the Hold ends.
    class Hold {
    public:
        explicit Hold(LinkStore& s) : s_(s) { work().hold = true; }
        ~Hold() {
            work().hold = false;
            s_.close(true);
        }
        Hold(const Hold&) = delete;
        Hold& operator=(const Hold&) = delete;

    private:
        LinkStore& s_;
    };
    static bool holding() { return work().hold; }

    // --- Saving and loading in the dense layout of the old arrays: blocks b0 .. b0 + n - 1 of
    // every voxel (voxel by voxel), the fast part or the slow part, each as one vector (its size,
    // then every block, zeros for empty ones). Loading: all fast parts come before the slow parts;
    // finishLoad() after the last.
    void stateKind(StateFile& st, const char* what, uint32_t b0, uint32_t n, bool slowPart) {
        if (slowPart && !slow_) { // no consolidation: an empty vector, as before
            uint64_t z = 0;
            st.value(z);
            if (!st.writing && z != 0) throw std::runtime_error(std::string("state file: size of ") + what + " differs");
            return;
        }
        uint64_t total = uint64_t(V_) * n * kBlock;
        st.value(total);
        if (!st.writing && total != uint64_t(V_) * n * kBlock)
            throw std::runtime_error(std::string("state file: size of ") + what + " differs");
        std::vector<float> buf(kBlock);
        if (!st.writing && !slowPart && b0 == 0) startLoad();
        for (size_t v = 0; v < V_; ++v)
            for (uint32_t b = b0; b < b0 + n; ++b) {
                if (st.writing) {
                    std::fill(buf.begin(), buf.end(), 0.0f);
                    const View x = view(v, b);
                    const float* src = slowPart ? x.slow : x.fast;
                    if (x.n && !x.pos) std::memcpy(buf.data(), src, kBlock * sizeof(float));
                    else
                        for (uint32_t k = 0; k < x.n; ++k) buf[x.pos[k]] = src[k];
                    st.bytes(buf.data(), kBlock * sizeof(float));
                } else {
                    st.bytes(buf.data(), kBlock * sizeof(float));
                    if (slowPart) loadSlow(v, b, buf.data());
                    else loadFast(v, b, buf.data());
                }
            }
    }
    void finishLoad() {
        for (Rec& r : rec_) {
            if (r.fast.empty()) {
                r = Rec();
                continue;
            }
            r.pos.shrink_to_fit(); // loading grew them step by step: give back the spare room
            r.fast.shrink_to_fit();
            r.slow.shrink_to_fit();
        }
    }

private:
    struct Rec {
        std::vector<uint32_t> off, poff; // per block (+1 at the end): where its values / positions start
        std::vector<uint8_t> pos;
        std::vector<float> fast, slow;
    };
    struct Work {
        AVec<float> buf;
        LinkStore* owner = nullptr;
        size_t v = SIZE_MAX;
        bool hold = false, dirty = false;
    };
    static Work& work() {
        thread_local Work w;
        return w;
    }
    static bool nonzero(float x) {
        uint32_t u;
        std::memcpy(&u, &x, 4);
        return u != 0; // -0 is kept too, so every bit comes back as it was
    }
    template <class T>
    static void copyInto(std::vector<T>& to, const std::vector<T>& from) {
        if (to.capacity() > 2 * from.size() + 256) std::vector<T>().swap(to); // give back what is far too much
        to.assign(from.begin(), from.end());
    }

    // Packs nb_ unpacked blocks into t, and zeroes the buffer.
    void pack(float* buf, Rec& t) const {
        t.off.assign(nb_ + 1, 0u);
        t.poff.assign(nb_ + 1, 0u);
        t.pos.clear();
        t.fast.clear();
        t.slow.clear();
        for (uint32_t b = 0; b < nb_; ++b) {
            float* f = buf + size_t(b) * 2 * kBlock;
            const float* s = f + kBlock;
            t.off[b] = uint32_t(t.fast.size());
            t.poff[b] = uint32_t(t.pos.size());
            uint32_t n = 0;
            for (size_t i = 0; i < kBlock; ++i) n += (nonzero(f[i]) || (slow_ && nonzero(s[i]))) ? 1u : 0u;
            if (!n) continue;
            if (n > kWhole) {
                t.fast.insert(t.fast.end(), f, f + kBlock);
                if (slow_) t.slow.insert(t.slow.end(), s, s + kBlock);
            } else
                for (size_t i = 0; i < kBlock; ++i)
                    if (nonzero(f[i]) || (slow_ && nonzero(s[i]))) {
                        t.pos.push_back(uint8_t(i));
                        t.fast.push_back(f[i]);
                        if (slow_) t.slow.push_back(s[i]);
                    }
            std::memset(f, 0, 2 * kBlock * sizeof(float));
        }
        t.off[nb_] = uint32_t(t.fast.size());
        t.poff[nb_] = uint32_t(t.pos.size());
    }

    // Room for `add` more, growing by an eighth at a time (not double: loading a big brain must
    // not need twice its memory on the way).
    template <class T>
    static void roomFor(std::vector<T>& v, size_t add) {
        if (v.size() + add > v.capacity()) v.reserve(v.size() + std::max(add, v.size() / 8 + kBlock));
    }
    void startLoad() {
        for (Rec& r : rec_) {
            r = Rec();
            r.off.assign(nb_ + 1, 0u);
            r.poff.assign(nb_ + 1, 0u);
        }
    }
    // The fast part of block b (blocks of a voxel arrive in order).
    void loadFast(size_t v, uint32_t b, const float* f) {
        Rec& r = rec_[v];
        r.off[b] = uint32_t(r.fast.size());
        r.poff[b] = uint32_t(r.pos.size());
        uint32_t n = 0;
        for (size_t i = 0; i < kBlock; ++i) n += nonzero(f[i]) ? 1u : 0u;
        const size_t add = n > kWhole ? kBlock : n;
        roomFor(r.fast, add);
        if (slow_) roomFor(r.slow, add);
        if (n <= kWhole) roomFor(r.pos, add);
        if (n > kWhole) r.fast.insert(r.fast.end(), f, f + kBlock);
        else
            for (size_t i = 0; i < kBlock; ++i)
                if (nonzero(f[i])) r.pos.push_back(uint8_t(i)), r.fast.push_back(f[i]);
        if (slow_) r.slow.resize(r.fast.size(), 0.0f);
        r.off[b + 1] = uint32_t(r.fast.size());
        r.poff[b + 1] = uint32_t(r.pos.size());
        for (uint32_t c = b + 1; c <= nb_; ++c) r.off[c] = r.off[b + 1], r.poff[c] = r.poff[b + 1];
    }
    // The slow part of block b: into the links the fast part made; a slow link where the fast one
    // is zero (rare) repacks the voxel.
    void loadSlow(size_t v, uint32_t b, const float* s) {
        Rec& r = rec_[v];
        const uint32_t o = r.off[b], n = r.off[b + 1] - o, p = r.poff[b];
        const bool whole = n && r.poff[b + 1] == p;
        if (whole) {
            std::memcpy(r.slow.data() + o, s, kBlock * sizeof(float));
            return;
        }
        int16_t at[kBlock];
        for (size_t i = 0; i < kBlock; ++i) at[i] = -1;
        for (uint32_t k = 0; k < n; ++k) at[r.pos[p + k]] = int16_t(k);
        bool fits = true;
        for (size_t i = 0; i < kBlock && fits; ++i) fits = !nonzero(s[i]) || at[i] >= 0;
        if (fits) {
            for (uint32_t k = 0; k < n; ++k) r.slow[o + k] = s[r.pos[p + k]];
            return;
        }
        float* buf = open(v);
        std::memcpy(buf + size_t(b) * 2 * kBlock + kBlock, s, kBlock * sizeof(float));
        close(true);
    }

    size_t V_ = 0;
    uint32_t nb_ = 0;
    bool slow_ = false;
    std::vector<Rec> rec_;
};

} // namespace ncm
