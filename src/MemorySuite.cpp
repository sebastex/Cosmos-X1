// Stage 1 memory suite (spec Section 10): the checks that make Stage 1 a foundation for
// Stage 2. Every test runs the same protocol on a learning matrix and on an identical
// matrix that never learns, and passes only if learning makes recall measurably more
// specific. Bars are fixed in advance.

#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

#include "ncm/Experiments.hpp"
#include "ncm/Lab.hpp"

namespace ncm {
namespace {

using lab::Session;
using lab::Specificity;

constexpr uint64_t kStore = 120; // 1D ticks per stored item
constexpr uint64_t kGap = 60;    // silence between items
constexpr uint64_t kCue = 60;    // 1D ticks per cue
constexpr float kCueFraction = 0.4f;
constexpr double kBar = 0.05;    // required specificity gain over the untrained twin

using Patterns = std::vector<std::vector<double>>;

Patterns storeAll(Session& s, const std::vector<std::string>& items, uint64_t storeTicks) {
    Patterns stored;
    for (const auto& item : items) {
        stored.push_back(s.present(item, storeTicks, 1.0f, true, storeTicks / 2));
        s.silence(kGap, true);
    }
    return stored;
}

Patterns recallAll(Session& s, const std::vector<std::string>& items, float fraction = kCueFraction) {
    Patterns recalled;
    for (const auto& item : items) {
        recalled.push_back(s.present(item, kCue, fraction, false, kCue / 2));
        s.silence(kGap, false);
    }
    return recalled;
}

struct Comparison {
    Specificity learned, untrained;
    double gain() const { return learned.margin - untrained.margin; }
    bool pass() const { return learned.identifies && gain() >= kBar; }
};

void printComparison(const char* label, const Comparison& c) {
    std::printf("  %-34s learned margin %+.3f (identifies all: %s) vs untrained %+.3f -> gain %+.3f (need >= %+.3f): %s\n",
                label, c.learned.margin, c.learned.identifies ? "yes" : "NO", c.untrained.margin, c.gain(), kBar,
                c.pass() ? "PASS" : "FAIL");
}

// Store the items, then cue each with a partial fingerprint, on both twins.
Comparison storeAndRecall(const Config& cfg, const std::vector<std::string>& items, uint64_t storeTicks = kStore) {
    Comparison c;
    for (int learning = 1; learning >= 0; --learning) {
        Session s(cfg, learning == 1);
        const Patterns stored = storeAll(s, items, storeTicks);
        const Patterns recalled = recallAll(s, items);
        (learning ? c.learned : c.untrained) = lab::specificity(lab::similarityMatrix(recalled, stored));
    }
    return c;
}

} // namespace

// CP3: with many memories stored, a fragment brings back its own memory, not a neighbour's.
int runCapacityTest(const Config& cfg) {
    const std::vector<std::string> items = {"a", "k", "z", "m", "q", "e", "t", "w"};
    std::printf("CP3 capacity: %zu memories stored, each recalled from a %.0f%% cue\n", items.size(), 100.0 * kCueFraction);
    const Comparison c = storeAndRecall(cfg, items);
    printComparison("8 memories:", c);
    return c.pass() ? 0 : 3;
}

// CP4: how much exposure a new memory needs. Passes if the memory is already usable after
// a short exposure (30 ticks, about 11 learning steps).
int runEfficiencyTest(const Config& cfg) {
    const std::vector<std::string> items = {"a", "k", "z"};
    std::printf("CP4 efficiency: memory strength after short and long exposure\n");
    bool shortPass = false;
    // (120 ticks is the recall test's own exposure, so it is not repeated here.)
    for (uint64_t ticks : {15ull, 30ull, 60ull}) {
        const Comparison c = storeAndRecall(cfg, items, ticks);
        char label[64];
        std::snprintf(label, sizeof(label), "exposure %llu ticks:", (unsigned long long)ticks);
        printComparison(label, c);
        if (ticks == 30) shortPass = c.pass();
    }
    std::printf("  memory usable after 30 ticks: %s\n", shortPass ? "PASS" : "FAIL");
    return shortPass ? 0 : 3;
}

// CP5: memories formed from streamed text (one character per tick, as language arrives).
int runStreamedTest(const Config& cfg) {
    const std::vector<std::string> items = {"apple ", "river ", "stone "};
    std::printf("CP5 streamed text: words streamed letter by letter, recalled from %.0f%% cues\n", 100.0 * kCueFraction);
    const Comparison c = storeAndRecall(cfg, items);
    printComparison("streamed words:", c);
    return c.pass() ? 0 : 3;
}

// CP6: continual learning. Old memories are stored and recalled, then new ones are learned,
// then everything is recalled. Old memories must survive the new learning, and both old
// and new must be specific.
int runContinualTest(const Config& cfg) {
    const std::vector<std::string> oldItems = {"a", "k", "z"};
    const std::vector<std::string> newItems = {"m", "q", "e"};
    std::vector<std::string> all = oldItems;
    all.insert(all.end(), newItems.begin(), newItems.end());
    std::printf("CP6 continual learning: learn 3, recall, learn 3 more, recall all 6\n");

    Comparison before, afterOld, afterNew, afterOldOnly;
    for (int learning = 1; learning >= 0; --learning) {
        Session s(cfg, learning == 1);
        Patterns stored = storeAll(s, oldItems, kStore);
        const Patterns recalledBefore = recallAll(s, oldItems);
        const Patterns storedNew = storeAll(s, newItems, kStore);
        stored.insert(stored.end(), storedNew.begin(), storedNew.end());
        const Patterns recalledAll = recallAll(s, all);

        Patterns oldStored(stored.begin(), stored.begin() + 3);
        (learning ? before.learned : before.untrained) =
            lab::specificity(lab::similarityMatrix(recalledBefore, oldStored));
        const auto sim = lab::similarityMatrix(recalledAll, stored);
        (learning ? afterOld.learned : afterOld.untrained) = lab::specificity(sim, {0, 1, 2});
        (learning ? afterNew.learned : afterNew.untrained) = lab::specificity(sim, {3, 4, 5});
        // Diagnostic: old cues against the old memories only (the same competitors as before),
        // which separates erasure of old memories from interference by the new ones.
        std::vector<std::vector<double>> oldOnly;
        for (size_t k = 0; k < 3; ++k) oldOnly.emplace_back(sim[k].begin(), sim[k].begin() + 3);
        (learning ? afterOldOnly.learned : afterOldOnly.untrained) = lab::specificity(oldOnly);
    }
    printComparison("old memories, before new learning:", before);
    printComparison("old memories, after new learning:", afterOld);
    printComparison("new memories:", afterNew);
    printComparison("  diagnostic, old vs old only:", afterOldOnly);
    std::printf("  diagnostic: erasure %+.3f (old vs old only), interference from new memories %+.3f\n",
                before.learned.margin - afterOldOnly.learned.margin,
                afterOldOnly.learned.margin - afterOld.learned.margin);
    const double forgetting = before.learned.margin - afterOld.learned.margin;
    const bool keeps = forgetting <= 0.05;
    std::printf("  forgetting of old memories: %+.3f (need <= +0.050): %s\n", forgetting, keeps ? "PASS" : "FAIL");
    return (before.pass() && afterOld.pass() && afterNew.pass() && keeps) ? 0 : 3;
}

// CP7 (early order check; order proper is Stage 2): after learning "a then k" and
// "z then m", cueing the first item of a pair should start to evoke the second more than
// cueing the second evokes the first, beyond what an untrained twin shows.
int runOrderTest(const Config& cfg) {
    const std::vector<std::pair<std::string, std::string>> pairs = {{"a", "k"}, {"z", "m"}};
    std::printf("CP7 early order check: learn 'a then k' and 'z then m', cue each item\n");

    double orderSignal[2] = {0.0, 0.0}; // [untrained, learned]: mean (forward - backward)
    double forward[2] = {0.0, 0.0};
    for (int learning = 1; learning >= 0; --learning) {
        Session s(cfg, learning == 1);
        for (int rep = 0; rep < 3; ++rep)
            for (const auto& [first, second] : pairs) {
                s.present(first, 40, 1.0f, true, UINT64_MAX);
                s.present(second, 40, 1.0f, true, UINT64_MAX);
                s.silence(40, true);
            }
        // Clean reference pattern for every item, learning off.
        auto reference = [&](const std::string& item) {
            auto p = s.present(item, 40, 1.0f, false, 20);
            s.silence(kGap, false);
            return p;
        };
        double sumForward = 0.0, sumSignal = 0.0;
        for (const auto& [first, second] : pairs) {
            const auto pFirst = reference(first), pSecond = reference(second);
            const auto cueFirst = s.present(first, 30, kCueFraction, false, 15);
            s.silence(kGap, false);
            const auto cueSecond = s.present(second, 30, kCueFraction, false, 15);
            s.silence(kGap, false);
            const double fwd = lab::cosine(cueFirst, pSecond);
            const double bwd = lab::cosine(cueSecond, pFirst);
            sumForward += fwd;
            sumSignal += fwd - bwd;
        }
        forward[learning] = sumForward / double(pairs.size());
        orderSignal[learning] = sumSignal / double(pairs.size());
    }
    const double fwdGain = forward[1] - forward[0];
    const double orderGain = orderSignal[1] - orderSignal[0];
    const bool pass = fwdGain >= 0.02 && orderGain >= 0.02;
    std::printf("  cue of first item evokes second: learned %.3f vs untrained %.3f (gain %+.3f, need >= +0.020)\n",
                forward[1], forward[0], fwdGain);
    std::printf("  forward minus backward:          learned %+.3f vs untrained %+.3f (gain %+.3f, need >= +0.020)\n",
                orderSignal[1], orderSignal[0], orderGain);
    std::printf("  early order signal: %s\n", pass ? "PASS" : "FAIL");
    return pass ? 0 : 3;
}

int runMemorySuite(const Config& cfg, bool earlyExit) {
    struct Entry {
        const char* name;
        int code;
    };
    std::vector<Entry> results;
    RecallOptions ro;
    std::printf("=== CP2 recall ===\n");
    double recallGain = 0.0;
    results.push_back({"CP2 recall (3 memories)", runRecallTest(cfg, ro, &recallGain)});
    if (earlyExit && recallGain < -0.02) {
        std::printf("\nsuite stopped early: learning makes recall worse than an untrained matrix (gain %+.3f)\n",
                    recallGain);
        return 3;
    }
    std::printf("\n=== CP3 ===\n");
    results.push_back({"CP3 capacity (8 memories)", runCapacityTest(cfg)});
    std::printf("\n=== CP4 ===\n");
    results.push_back({"CP4 efficiency", runEfficiencyTest(cfg)});
    std::printf("\n=== CP5 ===\n");
    results.push_back({"CP5 streamed text", runStreamedTest(cfg)});
    std::printf("\n=== CP6 ===\n");
    results.push_back({"CP6 continual learning", runContinualTest(cfg)});
    std::printf("\n=== CP7 ===\n");
    results.push_back({"CP7 early order", runOrderTest(cfg)});

    bool all = true;
    std::printf("\n=== Stage 1 memory suite summary ===\n");
    for (const auto& e : results) {
        std::printf("  %-28s %s\n", e.name, e.code == 0 ? "PASS" : "FAIL");
        all = all && e.code == 0;
    }
    return all ? 0 : 3;
}

} // namespace ncm

namespace ncm {

// Diagnostic: store one item, then hold its full input in recall mode and follow the 3D state
// tick by tick (similarity to the stored pattern, active voxels, total activity), on the
// learning matrix and its untrained twin. Shows whether recall starts right and drifts, or
// whether learned connections change the pattern from the start.
int runDriftTest(const Config& cfg, const std::string& item, uint64_t learnAfter) {
    std::printf("Recall drift: \"%s\" stored %llu ticks, then full input in recall mode\n", item.c_str(),
                (unsigned long long)kStore);
    std::vector<std::vector<std::string>> rows(2);
    for (int learning = 1; learning >= 0; --learning) {
        Session s(cfg, learning == 1);
        std::vector<double> stored;
        if (learnAfter > 0) { // diagnostic: no learning while the arrival wave passes
            s.setLearning(false);
            s.present(item, learnAfter, 1.0f, true, UINT64_MAX);
            s.setLearning(learning == 1);
            stored = s.present(item, kStore - learnAfter, 1.0f, true, kStore / 2 - std::min(learnAfter, kStore / 2));
        } else {
            stored = s.present(item, kStore, 1.0f, true, kStore / 2);
        }
        s.silence(kGap, true);
        for (uint64_t t = 0; t < kCue; ++t) {
            std::vector<double> now;
            s.present(item, 1, 1.0f, false, 0);
            s.accumulate(now);
            // Where the recalled activity differs from the stored pattern: activity on channels
            // that were silent in the stored pattern ("outside"), and the similarity on the
            // stored pattern's own channels (the shape of the pattern where it should be).
            double total = 0.0, outside = 0.0, dot = 0.0, nn = 0.0, ns = 0.0;
            for (size_t i = 0; i < now.size(); ++i) {
                total += now[i];
                if (stored[i] <= 0.0) {
                    outside += now[i];
                } else {
                    dot += now[i] * stored[i];
                    nn += now[i] * now[i];
                    ns += stored[i] * stored[i];
                }
            }
            if (t % 5 == 4) {
                char line[128];
                std::snprintf(line, sizeof(line), "sim %.3f outside %4.1f%% shape %.3f total %6.1f",
                              lab::cosine(now, stored), total > 0.0 ? 100.0 * outside / total : 0.0,
                              (nn > 0.0 && ns > 0.0) ? dot / std::sqrt(nn * ns) : 0.0, total);
                rows[learning].push_back(line);
            }
        }
    }
    std::printf("  tick  %-38s %s\n", "learned", "untrained");
    for (size_t k = 0; k < rows[0].size(); ++k)
        std::printf("  %4zu  %-38s %s\n", 5 * (k + 1), rows[1][k].c_str(), rows[0][k].c_str());
    return 0;
}

} // namespace ncm
