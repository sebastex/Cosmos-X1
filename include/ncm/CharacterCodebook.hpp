#pragma once
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace ncm {

// Fixed random sparse fingerprint per character (spec Section 6A).
// One table serves both the sensory surface (input) and the motor surface (output).
class CharacterCodebook {
public:
    // Printable ASCII (space to '~'): 95 symbols.
    static constexpr char32_t kFirst = U' ';
    static constexpr char32_t kLast = U'~';

    CharacterCodebook(size_t surface_lines, float sparsity, uint64_t seed);

    bool knows(char32_t c) const { return c >= kFirst && c <= kLast; }

    // Sorted surface line indices of a character's fingerprint. Unknown characters map to '?'.
    const std::vector<uint32_t>& fingerprint(char32_t c) const;

    // Readout by overlap (spec Section 6C): the character whose fingerprint overlaps
    // the active lines most, or U'\0' (silence) if none reaches `threshold` of its size.
    char32_t decode(const std::vector<uint32_t>& active_lines, float threshold = 0.30f) const;

    size_t linesPerCharacter() const { return lines_per_char_; }

private:
    size_t surface_lines_;
    size_t lines_per_char_;
    std::vector<std::vector<uint32_t>> table_;
};

} // namespace ncm
