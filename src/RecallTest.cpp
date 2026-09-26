#include <algorithm>
#include <array>
#include <climits>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <memory>
#include <vector>

#include "ncm/CharacterCodebook.hpp"
#include "ncm/Experiments.hpp"
#include "ncm/Lab.hpp"
#include "ncm/Matrix.hpp"

namespace ncm {
namespace {

using lab::cosine;
using lab::Session;
using lab::thin;

struct Result {
    std::vector<std::vector<double>> similarity; // [cue k][stored pattern j]
    std::array<double, kFields> ownByField{};    // mean similarity of each cue to its own pattern, per field
    std::array<double, kFields> otherByField{};  // mean similarity of each cue to the other patterns, per field
    std::vector<float> storeModulator;           // modulator at the end of each store phase
    LearningStats learning;                      // learning activity over the whole store phase
    double budget = 0.0;                         // total plastic strength after storing
    size_t voxels = 0;
};

Result runProtocol(const Config& cfg, RecallOptions opt, bool learning, float cueFraction) {
    opt.cueFraction = cueFraction;
    Session session(cfg, learning, opt.silenceSuppressed);
    const CharacterCodebook& cb = session.codebook();
    const size_t P = opt.patterns.size();
    Result r;
    std::vector<std::vector<double>> stored(P), recalled(P);

    // Store: each pattern streams with learning on; its reference is the mean
    // 3D activity over the second half of the presentation.
    for (size_t k = 0; k < P; ++k) {
        const std::string& text = opt.patterns[k];
        if (opt.clearBetween) session.clearActivity();
        for (uint64_t t = 0; t < opt.storeTicks; ++t) {
            const auto& fp = cb.fingerprint(char32_t(uint8_t(text[t % text.size()])));
            session.tick(&fp, &fp, true);
            if (t >= opt.storeTicks / 2) session.accumulate(stored[k]);
        }
        r.storeModulator.push_back(session.lastModulator());
        for (uint64_t t = 0; t < opt.gapTicks; ++t) session.tick(nullptr, nullptr, true);
    }
    r.learning = session.matrix().learningStats();
    r.budget = session.matrix().totalPlasticStrength();
    r.voxels = cfg.voxels();

    // Recall: partial cues, learning off.
    for (size_t k = 0; k < P; ++k) {
        const std::string& text = opt.patterns[k];
        if (opt.clearBetween) session.clearActivity();
        for (uint64_t t = 0; t < opt.cueTicks; ++t) {
            const char c = text[t % text.size()];
            const auto& full = cb.fingerprint(char32_t(uint8_t(c)));
            const auto cue = thin(full, opt.cueFraction, cfg.seed, c);
            session.tick(&cue, &full, false);
            if (t >= opt.cueTicks / 2) session.accumulate(recalled[k]);
        }
        for (uint64_t t = 0; t < opt.gapTicks; ++t) session.tick(nullptr, nullptr, false);
    }

    r.similarity.assign(P, std::vector<double>(P, 0.0));
    for (size_t k = 0; k < P; ++k)
        for (size_t j = 0; j < P; ++j) r.similarity[k][j] = cosine(recalled[k], stored[j]);

    const size_t perField = cfg.voxelsPerField() * C3;
    for (uint32_t f = 0; f < kFields; ++f) {
        double own = 0.0, other = 0.0;
        for (size_t k = 0; k < P; ++k)
            for (size_t j = 0; j < P; ++j) {
                const double s = cosine(recalled[k], stored[j], f * perField, (f + 1) * perField);
                (j == k ? own : other) += s;
            }
        r.ownByField[f] = own / double(P);
        r.otherByField[f] = P > 1 ? other / double(P * (P - 1)) : 0.0;
    }
    return r;
}

void printMatrix(const char* title, const RecallOptions& opt, const Result& r) {
    std::printf("%s (cosine similarity: rows = partial cue, columns = stored pattern)\n", title);
    std::printf("  %-10s", "");
    for (const auto& p : opt.patterns) std::printf("  %-8s", p.c_str());
    std::printf("\n");
    for (size_t k = 0; k < opt.patterns.size(); ++k) {
        std::printf("  %-10s", opt.patterns[k].c_str());
        for (double s : r.similarity[k]) std::printf("  %-8.3f", s);
        std::printf("\n");
    }
    static const char* names[kFields] = {"Input", "Memory", "Reasoning", "Output"};
    std::printf("  per field, own pattern vs others:");
    for (uint32_t f = 0; f < kFields; ++f)
        std::printf("  %s %.3f/%.3f", names[f], r.ownByField[f], r.otherByField[f]);
    std::printf("\n");
}

} // namespace

int runRecallTest(const Config& cfg, const RecallOptions& opt, double* specificityGain) {
    std::printf("Stage 1 recall test: %zu patterns, stored for %llu ticks each, recalled from %.0f%% cues\n\n",
                opt.patterns.size(), (unsigned long long)opt.storeTicks, 100.0 * opt.cueFraction);

    // Reliability first: without learning, does the same full input give the same pattern?
    // A matrix whose activity is not determined by its input cannot store anything.
    const Result reliability = runProtocol(cfg, opt, false, 1.0f);
    const Result learned = runProtocol(cfg, opt, true, opt.cueFraction);
    const Result baseline = runProtocol(cfg, opt, false, opt.cueFraction);

    printMatrix("Reliability (no learning, full input repeated)", opt, reliability);
    std::printf("\n");
    printMatrix("With learning", opt, learned);
    std::printf("  surprise modulator at the end of each store phase:");
    for (float m : learned.storeModulator) std::printf(" %.2f", m);
    std::printf("\n");
    const LearningStats& ls = learned.learning;
    std::printf("  learning: %llu updates, %.1f of %zu voxels learning per update; Hebbian change %.1f, "
                "removed by scaling %.1f; plastic strength after storing %.1f\n\n",
                (unsigned long long)ls.calls, ls.calls ? double(ls.learners) / double(ls.calls) : 0.0,
                learned.voxels, ls.change, ls.scaled, learned.budget);
    printMatrix("Without learning (identical matrix, learning off)", opt, baseline);

    const size_t P = opt.patterns.size();
    bool identifies = true;
    double ownLearned = 0.0, ownBaseline = 0.0, marginLearned = 0.0, marginBaseline = 0.0;
    for (size_t k = 0; k < P; ++k) {
        double otherL = 0.0, otherB = 0.0;
        for (size_t j = 0; j < P; ++j)
            if (j != k) {
                otherL = std::max(otherL, learned.similarity[k][j]);
                otherB = std::max(otherB, baseline.similarity[k][j]);
            }
        identifies = identifies && learned.similarity[k][k] > otherL;
        ownLearned += learned.similarity[k][k];
        ownBaseline += baseline.similarity[k][k];
        marginLearned += learned.similarity[k][k] - otherL;
        marginBaseline += baseline.similarity[k][k] - otherB;
    }
    ownLearned /= double(P);
    ownBaseline /= double(P);
    marginLearned /= double(P);
    marginBaseline /= double(P);

    // Recall must become more *specific*, not just stronger: a single attractor that every
    // cue falls into raises similarity to every pattern at once. So learning has to widen
    // the gap between a cue's own pattern and the others.
    const double gain = ownLearned - ownBaseline;
    const double marginGain = marginLearned - marginBaseline;
    const bool improves = marginGain >= 0.05;
    if (specificityGain) *specificityGain = marginGain;

    double relOwn = 0.0, relOther = 0.0;
    for (size_t k = 0; k < P; ++k)
        for (size_t j = 0; j < P; ++j) (j == k ? relOwn : relOther) += reliability.similarity[k][j];
    relOwn /= double(P);
    relOther /= double(P * (P - 1));
    const bool reliable = relOwn >= 0.5 && relOwn >= relOther + 0.2;

    std::printf("\nStage 1 check\n");
    std::printf("  input-driven (same input, same pattern): own %.3f vs others %.3f (need own >= 0.5 and >= others + 0.2): %s\n",
                relOwn, relOther, reliable ? "yes" : "NO");
    std::printf("  every cue recalls its own pattern:   %s\n", identifies ? "yes" : "NO");
    std::printf("  similarity to own pattern:           learned %.3f vs untrained %.3f (%+.3f)\n", ownLearned,
                ownBaseline, gain);
    std::printf("  specificity (own minus best other):  learned %+.3f vs untrained %+.3f (gain %+.3f, need >= +0.050): %s\n",
                marginLearned, marginBaseline, marginGain, improves ? "yes" : "NO");
    return (reliable && identifies && improves) ? 0 : 3;
}

} // namespace ncm
