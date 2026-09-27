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
    } else if (name == "small") {
        cfg.field_dim = 12;
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
        throw std::invalid_argument("unknown preset '" + name + "' (use tiny, small, dev or full)");
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
        {"line_upward_gain", &c.line_upward_gain},
        {"normalize_upward", &c.normalize_upward},
        {"output_sigma", &c.output_sigma},
        {"agc_rate", &c.agc_rate},
        {"agc_min", &c.agc_min},
        {"agc_max", &c.agc_max},
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
        {"long_range", &c.rule.long_range},
        {"link4d", &c.rule.link4d},
        {"link4d_backward", &c.rule.link4d_backward},
        {"learning_rate", &c.learning.rate},
        {"order_gain", &c.learning.order_gain},
        {"oja", &c.learning.oja},
        {"plastic_budget", &c.learning.plastic_budget},
        {"modulation_rate", &c.learning.modulation_rate},
        {"covariance", &c.learning.covariance},
        {"average_tau", &c.learning.average_tau},
        {"encoding_suppression", &c.learning.encoding_suppression},
        {"encoding_suppression_4d", &c.learning.encoding_suppression_4d},
        {"normalized_plasticity", &c.learning.normalized},
        {"soft_bound", &c.learning.soft_bound},
        {"predictive", &c.learning.predictive},
        {"trace_tau", &c.learning.trace_tau},
        {"modulator_tau", &c.learning.modulator_tau},
        {"fatigue_recall", &c.fatigue_recall},
        {"fire_threshold2", &c.fire_threshold2},
        {"fire_threshold3", &c.fire_threshold3},
        {"fire_gain2", &c.fire_gain2},
        {"fire_gain3", &c.fire_gain3},
        {"fatigue_gain2", &c.level2.fatigue_gain},
        {"fatigue_gain3", &c.level3.fatigue_gain},
        {"fatigue_tau2", &c.level2.fatigue_tau},
        {"fatigue_tau3", &c.level3.fatigue_tau},
    };
}

std::map<std::string, uint32_t*> countTable(Config& c) {
    return {
        // Geometry: any size can be set, not only the presets (scale invariance, spec Section 8A).
        {"field_dim", &c.field_dim},
        {"sheet_dim", &c.sheet_dim},
        {"line_len", &c.line_len},
        {"long_range_links", &c.long_range_links},
        {"winners2", &c.winners2},
        {"winners3", &c.winners3},
        {"inhibition_radius3", &c.inhibition_radius3},
        {"channel_winners2", &c.channel_winners2},
        {"channel_winners3", &c.channel_winners3},
    };
}

} // namespace

void applySetting(Config& cfg, const std::string& assignment) {
    const auto eq = assignment.find('=');
    if (eq == std::string::npos) throw std::invalid_argument("expected name=value, got '" + assignment + "'");
    const std::string name = assignment.substr(0, eq);
    const std::string value = assignment.substr(eq + 1);
    auto table = settingTable(cfg);
    if (const auto it = table.find(name); it != table.end()) {
        *it->second = std::stof(value);
        return;
    }
    auto counts = countTable(cfg);
    if (const auto it = counts.find(name); it != counts.end()) {
        *it->second = uint32_t(std::stoul(value));
        return;
    }
    throw std::invalid_argument("unknown setting '" + name + "'");
}

std::string settingNames() {
    Config c;
    std::string names;
    for (const auto& [name, ptr] : settingTable(c)) names += (names.empty() ? "" : ", ") + name;
    for (const auto& [name, ptr] : countTable(c)) names += ", " + name;
    return names;
}

} // namespace ncm
