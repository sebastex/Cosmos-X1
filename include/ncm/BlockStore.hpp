#pragma once
// Learned links stored only once they hold something (memory scales with what is learned).
//
// Every learned connection between two voxels is a C3 x C3 block of plastic weights (and, with
// consolidation, a slow block beside it). Most blocks never learn anything: they stay exactly
// zero for the brain's whole life. A block is therefore stored only from the moment it first
// becomes non-zero; until then it is "empty" and reads as zero. An empty block adds nothing to
// any sum, so the brain computes exactly what it computed with every block stored (proved by the
// brain fingerprint, NCM_HASH). Saved brain files keep the full (dense) layout, so old and new
// brains load into each other.
#include <atomic>
#include <cstdint>
#include <cstring>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <vector>

#include "ncm/Buffer.hpp"
#include "ncm/Config.hpp"
#include "ncm/State.hpp"

namespace ncm {

class BlockStore {
public:
    static constexpr size_t kBlock = size_t(C3) * C3; // floats in one block
    static constexpr size_t kPage = 1024;              // blocks per page

    BlockStore() = default;
    BlockStore(const BlockStore&) = delete;
    BlockStore& operator=(const BlockStore&) = delete;

    // `slots` blocks, all empty; `withSlow`: each block has a slow (consolidated) block beside it.
    void init(size_t slots, bool withSlow) {
        slots_ = slots;
        slow_ = withSlow;
        stride_ = kBlock * (withSlow ? 2 : 1);
        slot_.assign(slots, 0u);
        maxPages_ = slots / kPage + 2;
        pages_.reset(new std::atomic<float*>[maxPages_]);
        for (size_t p = 0; p < maxPages_; ++p) pages_[p].store(nullptr);
        owned_.clear();
        owned_.resize(maxPages_);
        next_.store(1);
    }
    bool empty() const { return slots_ == 0; }
    bool hasSlow() const { return slow_; }
    size_t slots() const { return slots_; }
    size_t stored() const { return size_t(next_.load()) - 1; } // blocks that hold something
    size_t bytes() const {
        size_t pages = 0;
        for (size_t p = 0; p < maxPages_; ++p) pages += owned_[p].empty() ? 0 : 1;
        return pages * kPage * stride_ * sizeof(float) + slot_.size() * sizeof(uint32_t);
    }

    // The block's weights (its slow block follows at + kBlock), or nullptr while it is empty.
    float* get(size_t i) const {
        const uint32_t s = slot_[i];
        return s ? pages_[s / kPage].load(std::memory_order_acquire) + size_t(s % kPage) * stride_ : nullptr;
    }
    // Stores block i (all zero) and returns it. Safe from many threads at once as long as each
    // block index is only ever touched by one thread (each voxel owns its incoming blocks).
    float* make(size_t i) {
        if (slot_[i]) return get(i);
        const uint32_t s = next_.fetch_add(1);
        if (size_t(s) >= (maxPages_ - 1) * kPage + kPage) throw std::runtime_error("BlockStore: out of slots");
        const size_t p = s / kPage;
        if (!pages_[p].load(std::memory_order_acquire)) {
            std::lock_guard<std::mutex> lock(mu_);
            if (!pages_[p].load(std::memory_order_relaxed)) {
                owned_[p].assign(kPage * stride_, 0.0f);
                pages_[p].store(owned_[p].data(), std::memory_order_release);
            }
        }
        slot_[i] = s;
        return pages_[p].load(std::memory_order_acquire) + size_t(s % kPage) * stride_;
    }

    // Save or restore in the dense layout of the old arrays: the fast blocks as one vector (its
    // size, then every block in order, zeros for empty ones), and the slow blocks as another.
    void stateFast(StateFile& st, const char* what) { dense(st, what, false); }
    void stateSlow(StateFile& st, const char* what) {
        if (!slow_) { // no consolidation: an empty vector, as before
            uint64_t n = 0;
            st.value(n);
            if (!st.writing && n != 0) throw std::runtime_error(std::string("state file: size of ") + what + " differs");
            return;
        }
        dense(st, what, true);
    }

private:
    void dense(StateFile& st, const char* what, bool slowPart) {
        uint64_t n = uint64_t(slots_) * kBlock;
        st.value(n);
        if (!st.writing && n != uint64_t(slots_) * kBlock)
            throw std::runtime_error(std::string("state file: size of ") + what + " differs");
        std::vector<float> buf(kBlock);
        for (size_t i = 0; i < slots_; ++i) {
            if (st.writing) {
                const float* b = get(i);
                if (b) std::memcpy(buf.data(), b + (slowPart ? kBlock : 0), kBlock * sizeof(float));
                else std::fill(buf.begin(), buf.end(), 0.0f);
                st.bytes(buf.data(), kBlock * sizeof(float));
            } else {
                st.bytes(buf.data(), kBlock * sizeof(float));
                bool any = false;
                for (float x : buf) any = any || x != 0.0f;
                if (any || get(i)) std::memcpy(make(i) + (slowPart ? kBlock : 0), buf.data(), kBlock * sizeof(float));
            }
        }
    }

    size_t slots_ = 0, stride_ = kBlock, maxPages_ = 0;
    bool slow_ = false;
    std::vector<uint32_t> slot_; // per block: 0 = empty, else its place in the pages
    std::unique_ptr<std::atomic<float*>[]> pages_;
    std::vector<AVec<float>> owned_;
    std::atomic<uint32_t> next_{1};
    std::mutex mu_;
};

} // namespace ncm
