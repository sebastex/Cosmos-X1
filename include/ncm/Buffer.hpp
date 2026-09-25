#pragma once
#include <cstddef>
#include <new>
#include <vector>

namespace ncm {

// 64-byte aligned storage so state and weight arrays suit SIMD loads and,
// later, zero-copy sharing with the integrated GPU.
template <class T, std::size_t Align = 64>
struct AlignedAllocator {
    using value_type = T;

    AlignedAllocator() noexcept = default;
    template <class U>
    AlignedAllocator(const AlignedAllocator<U, Align>&) noexcept {}

    T* allocate(std::size_t n) {
        return static_cast<T*>(::operator new(n * sizeof(T), std::align_val_t{Align}));
    }
    void deallocate(T* p, std::size_t) noexcept { ::operator delete(p, std::align_val_t{Align}); }

    template <class U>
    struct rebind {
        using other = AlignedAllocator<U, Align>;
    };

    friend bool operator==(const AlignedAllocator&, const AlignedAllocator&) { return true; }
    friend bool operator!=(const AlignedAllocator&, const AlignedAllocator&) { return false; }
};

template <class T>
using AVec = std::vector<T, AlignedAllocator<T>>;

// Double-buffered cell state for one level: cells read `cur` and write `next`,
// then the buffers swap, so every cell of a level updates from the same moment.
struct LevelState {
    AVec<float> cur;
    AVec<float> next;
    AVec<float> theta; // homeostatic threshold, one per cell (spec Section 3C)

    void allocate(std::size_t cells, std::size_t channels) {
        cur.assign(cells * channels, 0.0f);
        next.assign(cells * channels, 0.0f);
        theta.assign(cells, 0.0f);
    }
    void swap() { cur.swap(next); }
};

} // namespace ncm
