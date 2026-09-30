#include "ncm/Config.hpp"

#include <map>
#include <vector>
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

// The evolved rule (spec Section 5E: rule evolution replaces the hand-set starting rule).
// Adopted 2026-09-27 from the rate-regime search (tools/champion.json): the variant with the
// best mean fitness over its test seeds (small preset). Applied on top of every preset unless
// --rule starting is given; --set still overrides single values.
const std::vector<std::string>& evolvedRule() {
    // Foundation fixes adopted 2026-09-28 (applied after the evolved values): the volume-filling
    // 4D projection (deep fields use their whole volume; count scales with size) and divisive
    // fatigue in the sheets (weak steady input is never silenced).
    static const std::vector<std::string> rule = {
        "link4d_spread=16", "link4d_spread_scaled=1", "link4d_spread_share=0.5", "link4d_spread_gain=3.8069",
        "fatigue_divisive2=1",
        // Per-field gain control: each deep field keeps its activity near the target however
        // strong the input is (weak inputs left the deep fields silent, strong ones overdrove).
        "agc_rate=0.0646", "agc_max=16", "agc_relax_field=0.1264",
        // Competition, gain control and projection strength balanced by a short search
        // (tools/foundation_search.py) for steadiness under held input, volume use, repeatability
        // and recall at small and dev sizes, strong and weak inputs.
        "winners3=3",
        // Input-field depth: interior Input voxels hear random sensory-face voxels, so the Input
        // field uses its whole volume too (helps the bigger matrix most).
        "input_depth_spread=4", "input_depth_gain=0.179",
        // Streamed-word memory (adopted 2026-09-29, "version B"; diagnosed with --test completion,
        // chain, health and hum). Learned links reach 64 random partners per voxel (the fixed part
        // of those links keeps the old total), so sparse, scattered letter patterns can associate.
        // Anti-hub learning (full covariance, presynaptic budget). Strong learned links let a
        // word's start reactivate the whole word; they are kept in check by: gain control that
        // adapts only while there is input (no self-amplified echo), gain control also turning
        // down learned input, learned 4D links muted while encoding (no capture of new words onto
        // old cells), full muting of learned links in silence, and learned inhibition (each
        // voxel's inhibition learns to balance its excitation).
        "long_range_links=64", "presynaptic_bound=1",
        "agc_input_only=1", "agc_plastic=1", "encoding_suppression_4d=1",
        "istdp_rate=50",
        "channel_winners3=4",
        "covariance=1",
        "downward_gain=0.0224",
        "encoding_suppression=1",
        "fatigue_gain2=1.0101",
        "fatigue_gain3=0.0638",
        "fatigue_tau3=12.9342",
        "fire_gain2=4.8308",
        "fire_gain3=3.5738",
        "fire_threshold2=0.01",
        "fire_threshold3=0.0256",
        "hetero_ltd=1",
        "homeostasis3=0.0012",
        "learning_rate=0.04",
        "line_upward_gain=0.5199",
        "link4d=0.2482",
        "link4d_backward=0.0217",
        "long_range=0.00205",
        "mode_tau=30",
        "modulation_rate=0",
        "modulator_tau=0",
        "order_gain=4",
        "order_tau=5",
        "plastic_budget=4",
        "sheet_neighbour=0.0104",
        "sheet_self=0.1184",
        "soft_bound=1",
        "upward_gain=1.5216",
        "voxel_neighbour=0.0191",
        "voxel_self=0.0165"};
    return rule;
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
        {"link4d_spread_share", &c.rule.link4d_spread_share},
        {"link4d_spread_gain", &c.rule.link4d_spread_gain},
        {"input_depth_gain", &c.rule.input_depth_gain},
        {"learning_rate", &c.learning.rate},
        {"order_gain", &c.learning.order_gain},
        {"oja", &c.learning.oja},
        {"plastic_budget", &c.learning.plastic_budget},
        {"modulation_rate", &c.learning.modulation_rate},
        {"covariance", &c.learning.covariance},
        {"average_tau", &c.learning.average_tau},
        {"encoding_suppression", &c.learning.encoding_suppression},
        {"encoding_suppression_4d", &c.learning.encoding_suppression_4d},
        {"depression_use", &c.learning.depression_use},
        {"istdp_rate", &c.learning.istdp_rate},
        {"istdp_target", &c.learning.istdp_target},
        {"istdp_max", &c.learning.istdp_max},
        {"istdp_tau", &c.learning.istdp_tau},
        {"depression_tau", &c.learning.depression_tau},
        {"normalized_plasticity", &c.learning.normalized},
        {"soft_bound", &c.learning.soft_bound},
        {"predictive", &c.learning.predictive},
        {"trace_tau", &c.learning.trace_tau},
        {"modulator_tau", &c.learning.modulator_tau},
        {"order_tau", &c.learning.order_tau},
        {"mode_tau", &c.learning.mode_tau},
        {"hetero_ltd", &c.learning.hetero_ltd},
        {"presynaptic_bound", &c.learning.presynaptic_bound},
        {"consolidation_rate", &c.learning.consolidation_rate},
        {"sheet_rate", &c.learning.sheet_rate},
        {"sheet_budget", &c.learning.sheet_budget},
        {"consolidated_budget", &c.learning.consolidated_budget},
        {"fatigue_recall", &c.fatigue_recall},
        {"fatigue_divisive", &c.fatigue_divisive},
        {"fatigue_cap", &c.fatigue_cap},
        {"fatigue_divisive2", &c.fatigue_divisive2},
        {"link4d_spread_scaled", &c.link4d_spread_scaled},
        {"agc_local", &c.agc_local},
        {"agc_relax", &c.agc_relax},
        {"agc_relax_field", &c.agc_relax_field},
        {"agc_plastic", &c.agc_plastic},
        {"agc_input_only", &c.agc_input_only},
        {"field_pace", &c.field_pace},
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
        {"codebook_seed", &c.codebook_seed},
        {"link4d_spread", &c.link4d_spread},
        {"agc_radius", &c.agc_radius},
        {"input_depth_spread", &c.input_depth_spread},
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

void applyEvolvedRule(Config& cfg) {
    for (const auto& a : evolvedRule()) applySetting(cfg, a);
}

} // namespace ncm
