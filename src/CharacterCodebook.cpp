#include "ncm/CharacterCodebook.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>

#include "ncm/Random.hpp"

namespace ncm {

CharacterCodebook::CharacterCodebook(size_t surface_lines, float sparsity, uint64_t seed)
    : surface_lines_(surface_lines) {
    lines_per_char_ = std::max<size_t>(1, size_t(std::lround(double(surface_lines) * sparsity)));
    if (lines_per_char_ > surface_lines) throw std::invalid_argument("fingerprint larger than the surface");

    table_.resize(size_t(kLast - kFirst) + 1);
    std::vector<uint8_t> taken(surface_lines);
    for (size_t i = 0; i < table_.size(); ++i) {
        Rng rng(mix64(seed ^ mix64(kStreamCodebook * 0x10000ull + i)));
        std::fill(taken.begin(), taken.end(), uint8_t(0));
        auto& fp = table_[i];
        fp.reserve(lines_per_char_);
        while (fp.size() < lines_per_char_) {
            const auto s = uint32_t(rng.below(surface_lines));
            if (!taken[s]) {
                taken[s] = 1;
                fp.push_back(s);
            }
        }
        std::sort(fp.begin(), fp.end());
    }
}

const std::vector<uint32_t>& CharacterCodebook::fingerprint(char32_t c) const {
    if (!knows(c)) c = U'?';
    return table_[size_t(c - kFirst)];
}

char32_t CharacterCodebook::decode(const std::vector<uint32_t>& active_lines, float threshold) const {
    if (active_lines.empty()) return U'\0';
    std::vector<uint8_t> active(surface_lines_, 0);
    for (uint32_t s : active_lines)
        if (s < surface_lines_) active[s] = 1;

    size_t best = 0, bestOverlap = 0;
    for (size_t i = 0; i < table_.size(); ++i) {
        size_t overlap = 0;
        for (uint32_t s : table_[i]) overlap += active[s];
        if (overlap > bestOverlap) {
            bestOverlap = overlap;
            best = i;
        }
    }
    if (double(bestOverlap) < double(threshold) * double(lines_per_char_)) return U'\0';
    return kFirst + char32_t(best);
}

} // namespace ncm
