#include "ncm/Config.hpp"

#include <map>
#include <stdexcept>

namespace ncm {

Config makePreset(const std::string& name) {
    Config cfg;
    if (name == "tiny") {
        cfg.field_dim = 8;
        cfg.sheet_dim = 8;
        cfg.line_len = 16;
    } else if (name == "dev") {
        cfg.field_dim = 16;
        cfg.sheet_dim = 8;
        cfg.line_len = 16;
    } else if (name == "full") {
        cfg.field_dim = 32;
        cfg.sheet_dim = 16;
        cfg.line_len = 16;
    } else {
        throw std::invalid_argument("unknown preset '" + name + "' (use tiny, dev or full)");
    }
    return cfg;
}

namespace {

std::map<std::string, float*> settingTable(Config& c) {
    return {
        {"target_activity", &c.target_activity},
        {"inhibitory_share", &c.inhibitory_share},
        {"inhibitory_strength", &c.inhibitory_strength},
        {"upward_gain", &c.upward_gain},
        {"downward_gain", &c.downward_gain},
        {"homeostasis1", &c.level1.homeostasis_rate},
        {"homeostasis2", &c.level2.homeostasis_rate},
        {"homeostasis3", &c.level3.homeostasis_rate},
        {"active_level1", &c.level1.active_level},
        {"active_level2", &c.level2.active_level},
        {"active_level3", &c.level3.active_level},
        {"line_carry", &c.rule.line_carry},
        {"sheet_self", &c.rule.sheet_self},
        {"sheet_neighbour", &c.rule.sheet_neighbour},
        {"voxel_self", &c.rule.voxel_self},
        {"voxel_neighbour", &c.rule.voxel_neighbour},
        {"voxel_noise", &c.rule.voxel_noise},
        {"long_range", &c.rule.long_range},
        {"link4d", &c.rule.link4d},
    };
}

} // namespace

void applySetting(Config& cfg, const std::string& assignment) {
    const auto eq = assignment.find('=');
    if (eq == std::string::npos) throw std::invalid_argument("expected name=value, got '" + assignment + "'");
    const std::string name = assignment.substr(0, eq);
    auto table = settingTable(cfg);
    const auto it = table.find(name);
    if (it == table.end()) throw std::invalid_argument("unknown setting '" + name + "'");
    *it->second = std::stof(assignment.substr(eq + 1));
}

std::string settingNames() {
    Config c;
    std::string names;
    for (const auto& [name, ptr] : settingTable(c)) names += (names.empty() ? "" : ", ") + name;
    return names;
}

} // namespace ncm
