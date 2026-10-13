#pragma once
#include <string>
#include <vector>

namespace ncm {

// Word lists B, C, ... (300 unique words each; list A is the pair test's own list).
const std::vector<std::vector<std::string>>& extraWordLists();

} // namespace ncm
