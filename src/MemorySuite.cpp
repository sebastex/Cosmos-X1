// Stage 1 memory suite (spec Section 10): the checks that make Stage 1 a foundation for
// Stage 2. Every test runs the same protocol on a learning matrix and on an identical
// matrix that never learns, and passes only if learning makes recall measurably more
// specific. Bars are fixed in advance.

#include <cmath>
#include <numeric>
#include <algorithm>
#include <cstdio>
#include <string>
#include <vector>

#include "ncm/Experiments.hpp"
#include "ncm/Lab.hpp"

namespace ncm {
namespace {

using lab::Session;
using lab::Specificity;

// Test durations in 1D ticks, all multiplied by the test time scale (setTestTimeScale): a
// bigger matrix takes longer to settle, so tests of scale invariance stretch them with size.
uint64_t kStore = 120; // per stored item
uint64_t kGap = 60;    // silence between items
uint64_t kCue = 60;    // per cue
double gScale = 1.0;
uint64_t T(uint64_t ticks) { return uint64_t(double(ticks) * gScale + 0.5); }
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

void setTestTimeScale(double scale) {
    gScale = scale > 0.0 ? scale : 1.0;
    kStore = T(120);
    kGap = T(60);
    kCue = T(60);
}

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
    for (uint64_t ticks : {T(15), T(30), T(60)}) {
        const Comparison c = storeAndRecall(cfg, items, ticks);
        char label[64];
        std::snprintf(label, sizeof(label), "exposure %llu ticks:", (unsigned long long)ticks);
        printComparison(label, c);
        if (ticks == T(30)) shortPass = c.pass();
    }
    std::printf("  memory usable after 30 ticks: %s\n", shortPass ? "PASS" : "FAIL");
    return shortPass ? 0 : 3;
}

// CP5: memories formed from streamed text (one character per tick, as language arrives).
int runStreamedTest(const Config& cfg) {
    const std::vector<std::string> items = {"apple ", "river ", "stone "};
    std::printf("CP5 streamed text: words streamed letter by letter, recalled from %.0f%% cues\n", 100.0 * kCueFraction);
    Comparison c;
    std::vector<std::vector<double>> sims[2];
    double selfFlow[3] = {}, crossFlow[3] = {};
    for (int learning = 1; learning >= 0; --learning) {
        Session s(cfg, learning == 1);
        const Patterns stored = storeAll(s, items, kStore);
        if (learning == 1)
            for (size_t k = 0; k < 3; ++k) {
                selfFlow[k] = s.flow(stored[k], stored[k]);
                for (size_t j = 0; j < 3; ++j)
                    if (j != k) crossFlow[k] = std::max(crossFlow[k], s.flow(stored[k], stored[j]));
            }
        const Patterns recalled = recallAll(s, items);
        sims[learning] = lab::similarityMatrix(recalled, stored);
        (learning ? c.learned : c.untrained) = lab::specificity(sims[learning]);
    }
    printComparison("streamed words:", c);
    // Diagnostics: which stored word each cue resembles, and the learned links within and
    // between words (stored pattern -> stored pattern).
    for (int l = 1; l >= 0; --l) {
        std::printf("  diagnostic, %s: cue vs stored (apple river storm)\n", l ? "learned" : "untrained");
        for (size_t k = 0; k < 3; ++k)
            std::printf("    %-6s %.3f %.3f %.3f\n", items[k].c_str(), sims[l][k][0], sims[l][k][1], sims[l][k][2]);
    }
    std::printf("  diagnostic, stored links within each word: %.4f %.4f %.4f; strongest into another word: %.4f %.4f %.4f\n",
                selfFlow[0], selfFlow[1], selfFlow[2], crossFlow[0], crossFlow[1], crossFlow[2]);
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
    std::vector<std::vector<double>> simAfter[2], simBefore[2]; // [untrained, learned]
    double selfBefore[3] = {}, selfAfter[3] = {}, crossAfter[3] = {}; // learned twin: stored links
    for (int learning = 1; learning >= 0; --learning) {
        Session s(cfg, learning == 1);
        Patterns stored = storeAll(s, oldItems, kStore);
        if (learning == 1)
            for (size_t k = 0; k < 3; ++k) selfBefore[k] = s.flow(stored[k], stored[k]);
        const Patterns recalledBefore = recallAll(s, oldItems);
        const Patterns storedNew = storeAll(s, newItems, kStore);
        stored.insert(stored.end(), storedNew.begin(), storedNew.end());
        if (learning == 1)
            for (size_t k = 0; k < 3; ++k) {
                selfAfter[k] = s.flow(stored[k], stored[k]);
                for (size_t j = 3; j < 6; ++j) crossAfter[k] = std::max(crossAfter[k], s.flow(stored[k], stored[j]));
            }
        const Patterns recalledAll = recallAll(s, all);

        Patterns oldStored(stored.begin(), stored.begin() + 3);
        (learning ? before.learned : before.untrained) =
            lab::specificity(lab::similarityMatrix(recalledBefore, oldStored));
        const auto sim = lab::similarityMatrix(recalledAll, stored);
        simAfter[learning] = sim;
        simBefore[learning] = lab::similarityMatrix(recalledBefore, oldStored);
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
    std::printf("  diagnostic, stored links of each old memory (own pattern -> itself), before -> after new learning:");
    for (size_t k = 0; k < 3; ++k) std::printf("  %.4f -> %.4f", selfBefore[k], selfAfter[k]);
    std::printf("\n  diagnostic, strongest link from each old memory into a new one:");
    for (size_t k = 0; k < 3; ++k) std::printf("  %.4f", crossAfter[k]);
    std::printf("\n");
    // Diagnostic: which stored memory each old cue resembles, before and after new learning.
    const char* names[6] = {"a", "k", "z", "m", "q", "e"};
    for (int l = 1; l >= 0; --l) {
        std::printf("  diagnostic, %s: old cue vs stored  (before: a k z | after: a k z m q e)\n",
                    l ? "learned" : "untrained");
        for (size_t k = 0; k < 3; ++k) {
            std::printf("    cue %s  before", names[k]);
            for (size_t j = 0; j < 3; ++j) std::printf(" %.3f", simBefore[l][k][j]);
            std::printf("  | after");
            for (size_t j = 0; j < 6; ++j) std::printf(" %.3f", simAfter[l][k][j]);
            std::printf("\n");
        }
    }
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
// "z then m", cueing the first item of a pair should evoke the second more than cueing the
// second evokes the first, beyond what an untrained twin shows. The evoked item is read in
// the 30 ticks right after the cue: sequence memory plays forward once the cue ends (while
// the cue is shown its own item dominates). The reading during the cue is still printed.
int runOrderTest(const Config& cfg) {
    // Three independent sessions of two pairs each (fresh matrices, different letters),
    // averaged: with one session of two pairs, chance asymmetries between the items (visible
    // in the untrained twin, about +-0.05) exceeded the bar. Each session is the same task.
    const std::vector<std::vector<std::pair<std::string, std::string>>> sets = {
        {{"a", "k"}, {"z", "m"}}, {{"q", "e"}, {"t", "w"}}, {{"b", "p"}, {"r", "s"}}};
    std::printf("CP7 early order check: 3 sessions, each learns two pairs (first then second), cue each item\n");

    double orderSignal[2] = {0.0, 0.0}; // [untrained, learned]: mean (forward - backward)
    double storedForward = 0.0, storedBackward = 0.0;   // diagnostic: learned links, learned twin
    double afterForward[2] = {0.0, 0.0}, afterSignal[2] = {0.0, 0.0}; // right after the cue
    double afterSpecific[2] = {0.0, 0.0}; // own partner minus the other pair's second item
    double forward[2] = {0.0, 0.0};
    const double share = 1.0 / double(sets.size());
    for (const auto& pairs : sets)
    for (int learning = 1; learning >= 0; --learning) {
        Session s(cfg, learning == 1);
        for (int rep = 0; rep < 3; ++rep)
            for (const auto& [first, second] : pairs) {
                s.present(first, T(40), 1.0f, true, UINT64_MAX);
                s.present(second, T(40), 1.0f, true, UINT64_MAX);
                s.silence(T(40), true);
            }
        // Clean reference pattern for every item: encoding mode (as the other checks store
        // their references) with learning paused. In recall mode the learned forward link
        // leaked the second item into the first item's reference (audit 2026-09-27).
        auto reference = [&](const std::string& item) {
            s.setLearning(false);
            auto p = s.present(item, T(40), 1.0f, true, T(20));
            s.silence(kGap, true);
            s.setLearning(learning == 1);
            return p;
        };
        std::vector<std::vector<double>> refFirst, refSecond;
        for (const auto& [first, second] : pairs) {
            refFirst.push_back(reference(first));
            refSecond.push_back(reference(second));
        }
        double sumForward = 0.0, sumSignal = 0.0;
        for (size_t pi = 0; pi < pairs.size(); ++pi) {
            const auto& [first, second] = pairs[pi];
            const auto& pFirst = refFirst[pi];
            const auto& pSecond = refSecond[pi];
            const auto& pOtherSecond = refSecond[(pi + 1) % pairs.size()];
            if (learning == 1) {
                storedForward += share * s.flow(pFirst, pSecond) / double(pairs.size());
                storedBackward += share * s.flow(pSecond, pFirst) / double(pairs.size());
            }
            const auto cueFirst = s.present(first, T(30), kCueFraction, false, T(15));
            const auto afterFirst = s.silence(T(30), false, 0);
            s.silence(kGap - T(30), false);
            const auto cueSecond = s.present(second, T(30), kCueFraction, false, T(15));
            const auto afterSecond = s.silence(T(30), false, 0);
            s.silence(kGap - T(30), false);
            const double aFwd = lab::cosine(afterFirst, pSecond);
            afterForward[learning] += share * aFwd / double(pairs.size());
            afterSignal[learning] += share * (aFwd - lab::cosine(afterSecond, pFirst)) / double(pairs.size());
            // The evoked item must be the cue's own partner, not any item that followed something.
            afterSpecific[learning] += share * (aFwd - lab::cosine(afterFirst, pOtherSecond)) / double(pairs.size());
            const double fwd = lab::cosine(cueFirst, pSecond);
            const double bwd = lab::cosine(cueSecond, pFirst);
            sumForward += fwd;
            sumSignal += fwd - bwd;
        }
        forward[learning] += share * sumForward / double(pairs.size());
        orderSignal[learning] += share * sumSignal / double(pairs.size());
    }
    const double fwdGain = afterForward[1] - afterForward[0];
    const double orderGain = afterSignal[1] - afterSignal[0];
    const double specificGain = afterSpecific[1] - afterSpecific[0];
    const bool pass = fwdGain >= 0.02 && orderGain >= 0.02 && specificGain >= 0.02;
    std::printf("  cue of first item evokes second: learned %.3f vs untrained %.3f (gain %+.3f, need >= +0.020)\n",
                afterForward[1], afterForward[0], fwdGain);
    std::printf("  forward minus backward:          learned %+.3f vs untrained %+.3f (gain %+.3f, need >= +0.020)\n",
                afterSignal[1], afterSignal[0], orderGain);
    std::printf("  own partner minus other pair's second: learned %+.3f vs untrained %+.3f (gain %+.3f, need >= +0.020)\n",
                afterSpecific[1], afterSpecific[0], specificGain);
    std::printf("  diagnostic, during the cue: first evokes second %.3f vs untrained %.3f (gain %+.3f); "
                "forward minus backward %+.3f vs %+.3f (gain %+.3f)\n",
                forward[1], forward[0], forward[1] - forward[0], orderSignal[1], orderSignal[0],
                orderSignal[1] - orderSignal[0]);
    std::printf("  diagnostic, stored links: first->second %.4f, second->first %.4f (ratio %.2f)\n",
                storedForward, storedBackward, storedBackward > 0.0 ? storedForward / storedBackward : 0.0);
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
    std::printf("\n=== CP5b ===\n");
    results.push_back({"CP5b word completion", runCompletionTest(cfg, 3)});
    std::printf("\n=== CP6 ===\n");
    results.push_back({"CP6 continual learning", runContinualTest(cfg)});
    std::printf("\n=== CP6b ===\n");
    results.push_back({"CP6b retention (8+16)", runRetentionTest(cfg)});
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
    // Amplitude of 3D activity while the item is held (untrained twin): the strongest channel
    // per voxel, per field. A voxel counts as firing from the active level (0.5).
    {
        Session s(cfg, false);
        s.present(item, kStore, 1.0f, false, UINT64_MAX);
        const auto& v = s.matrix().voxelState();
        const size_t perField = v.size() / kFields;
        std::printf("  amplitude while held (untrained): strongest channel per voxel\n");
        for (uint32_t f = 0; f < kFields; ++f) {
            float top = 0.0f;
            size_t above[4] = {};
            const float levels[4] = {0.01f, 0.05f, 0.1f, 0.5f};
            for (size_t i = f * perField; i < (f + 1) * perField; i += C3) {
                float strongest = 0.0f;
                for (uint32_t c = 0; c < C3; ++c) strongest = std::max(strongest, v[i + c]);
                top = std::max(top, strongest);
                for (int k = 0; k < 4; ++k) above[k] += strongest >= levels[k];
            }
            std::printf("    field %u: max %.3f; voxels >= 0.01: %zu, >= 0.05: %zu, >= 0.1: %zu, >= 0.5: %zu (of %zu)\n",
                        f, top, above[0], above[1], above[2], above[3], perField / C3);
        }
    }
    std::printf("  tick  %-38s %s\n", "learned", "untrained");
    for (size_t k = 0; k < rows[0].size(); ++k)
        std::printf("  %4zu  %-38s %s\n", 5 * (k + 1), rows[1][k].c_str(), rows[0][k].c_str());
    return 0;
}

// Diagnostic: context coding. Streams each word (no learning) and averages the 3D state at
// the ticks where the shared letter 'e' (or the space) is the current input. If "e in apple"
// and "e in river" give nearly the same pattern, the matrix has no context for shared letters
// and Hebbian association from them cannot stay word-specific. Printed per field, with the
// same word's two halves as a reliability reference.
int runContextTest(const Config& cfg) {
    const std::vector<std::string> words = {"apple ", "river ", "stone "};
    const char shared[2] = {'e', ' '};
    std::printf("Context coding: 3D state when a shared letter is current, compared across words\n");
    for (char letter : shared) {
        std::vector<std::vector<double>> first(3), second(3);
        for (size_t w = 0; w < words.size(); ++w) {
            Session s(cfg, false);
            const std::string& text = words[w];
            for (uint64_t t = 0; t < 240; ++t) {
                const char c = text[t % text.size()];
                const auto& fp = s.codebook().fingerprint(char32_t(uint8_t(c)));
                s.tick(&fp, &fp, false);
                if (t >= 60 && c == letter) s.accumulate(t < 150 ? first[w] : second[w]);
            }
        }
        const size_t perField = first[0].size() / kFields;
        std::printf("  letter '%c':\n", letter == ' ' ? '_' : letter);
        for (uint32_t f = 0; f < kFields; ++f) {
            const size_t b = f * perField, e = (f + 1) * perField;
            double self = 0.0, cross = 0.0;
            int nc = 0;
            for (size_t w = 0; w < 3; ++w) {
                self += lab::cosine(first[w], second[w], b, e) / 3.0;
                for (size_t v = 0; v < 3; ++v)
                    if (v != w) {
                        cross += lab::cosine(first[w], second[v], b, e);
                        ++nc;
                    }
            }
            std::printf("    field %u: same word %.3f, different words %.3f\n", f, self, cross / nc);
        }
    }
    return 0;
}

// Memory profile (harsher, more informative than the pass/fail checks): what recall actually
// produces. A: recall of 8 memories from 20/40/60/80% cues (accuracy = share of cues whose best
// match is their own memory). B: conflicting cues (40% of one memory + 40% of another): clean
// choice or blend. C: are intrusions between memories related to how similar their inputs
// were (generalization) or unrelated (noise)? D: forgetting curve of 8 memories while 4, 8 and
// 16 more are learned. Learning matrix vs untrained twin throughout.
namespace {

struct RecallStats {
    double accuracy = 0.0, own = 0.0, other = 0.0;
};

RecallStats recallStats(Session& s, const std::string& items, const Patterns& stored, float fraction) {
    RecallStats r;
    const size_t n = items.size();
    for (size_t k = 0; k < n; ++k) {
        const auto rec = s.present(std::string(1, items[k]), kCue, fraction, false, kCue / 2);
        s.silence(kGap, false);
        double own = lab::cosine(rec, stored[k]), best = -1.0;
        for (size_t j = 0; j < stored.size(); ++j)
            if (j != k) best = std::max(best, lab::cosine(rec, stored[j]));
        r.accuracy += (own > best) ? 1.0 : 0.0;
        r.own += own;
        r.other += best;
    }
    r.accuracy /= double(n);
    r.own /= double(n);
    r.other /= double(n);
    return r;
}

std::vector<double> presentMixed(Session& s, char a, char b, float fraction, uint64_t seed) {
    std::vector<double> acc;
    for (uint64_t t = 0; t < kCue; ++t) {
        auto cue = lab::thin(s.codebook().fingerprint(char32_t(uint8_t(a))), fraction, seed, a);
        const auto other = lab::thin(s.codebook().fingerprint(char32_t(uint8_t(b))), fraction, seed, b);
        cue.insert(cue.end(), other.begin(), other.end());
        std::sort(cue.begin(), cue.end());
        cue.erase(std::unique(cue.begin(), cue.end()), cue.end());
        s.tick(&cue, nullptr, false);
        if (t >= kCue / 2) s.accumulate(acc);
    }
    s.silence(kGap, false);
    return acc;
}

} // namespace

int runProfileTest(const Config& cfg) {
    const std::string items = "akzmqetw";
    std::printf("Memory profile: %zu memories, learning matrix vs untrained twin\n", items.size());
    Patterns storedBoth[2];
    std::vector<std::vector<double>> sim40[2];
    for (int learning = 1; learning >= 0; --learning) {
        Session s(cfg, learning == 1);
        std::vector<std::string> list;
        for (char c : items) list.emplace_back(1, c);
        const Patterns stored = storeAll(s, list, kStore);
        storedBoth[learning] = stored;
        std::printf("  %s\n", learning ? "LEARNED" : "UNTRAINED");
        std::printf("    A. cue strength -> accuracy (right memory is best match), own similarity, best other\n");
        for (float f : {0.2f, 0.4f, 0.6f, 0.8f}) {
            const RecallStats r = recallStats(s, items, stored, f);
            std::printf("       %3.0f%% cue: accuracy %3.0f%%  own %.3f  best other %.3f\n", 100.0 * f, 100.0 * r.accuracy,
                        r.own, r.other);
        }
        // 40% cue similarity table for C.
        sim40[learning].assign(items.size(), std::vector<double>(items.size(), 0.0));
        for (size_t k = 0; k < items.size(); ++k) {
            const auto rec = s.present(std::string(1, items[k]), kCue, 0.4f, false, kCue / 2);
            s.silence(kGap, false);
            for (size_t j = 0; j < items.size(); ++j) sim40[learning][k][j] = lab::cosine(rec, stored[j]);
        }
        std::printf("    B. conflicting cues (40%% of X + 40%% of Y): similarity to X, to Y, best other\n");
        const char pairs[4][2] = {{'a', 'k'}, {'z', 'm'}, {'q', 'e'}, {'t', 'w'}};
        for (const auto& pr : pairs) {
            const auto rec = presentMixed(s, pr[0], pr[1], 0.4f, cfg.itemSeed());
            const size_t ix = items.find(pr[0]), iy = items.find(pr[1]);
            double best = -1.0;
            for (size_t j = 0; j < items.size(); ++j)
                if (j != ix && j != iy) best = std::max(best, lab::cosine(rec, stored[j]));
            std::printf("       %c+%c: %.3f %.3f  other %.3f\n", pr[0], pr[1], lab::cosine(rec, stored[ix]),
                        lab::cosine(rec, stored[iy]), best);
        }
    }
    // C: intrusions vs input similarity (untrained stored patterns = what the inputs look like).
    {
        std::vector<double> xs, ys;
        for (size_t k = 0; k < items.size(); ++k)
            for (size_t j = 0; j < items.size(); ++j)
                if (j != k) {
                    xs.push_back(lab::cosine(storedBoth[0][k], storedBoth[0][j]));
                    ys.push_back(sim40[1][k][j] - sim40[0][k][j]); // intrusion added by learning
                }
        const double mx = std::accumulate(xs.begin(), xs.end(), 0.0) / double(xs.size());
        const double my = std::accumulate(ys.begin(), ys.end(), 0.0) / double(ys.size());
        double num = 0.0, dx = 0.0, dy = 0.0;
        for (size_t i = 0; i < xs.size(); ++i) {
            num += (xs[i] - mx) * (ys[i] - my);
            dx += (xs[i] - mx) * (xs[i] - mx);
            dy += (ys[i] - my) * (ys[i] - my);
        }
        std::printf("  C. intrusion added by learning vs input similarity of the two memories: correlation %+.2f "
                    "(mean added intrusion %+.3f)\n", (dx > 0 && dy > 0) ? num / std::sqrt(dx * dy) : 0.0, my);
    }
    // D: forgetting curve.
    {
        const std::string batches[3] = {"bdfg", "hjlp", "nosuvxyc"};
        for (int learning = 1; learning >= 0; --learning) {
            Session s(cfg, learning == 1);
            std::vector<std::string> list;
            for (char c : items) list.emplace_back(1, c);
            const Patterns stored = storeAll(s, list, kStore);
            std::printf("  D. forgetting curve (%s): 8 old memories, 40%% cues\n", learning ? "learned" : "untrained");
            RecallStats r = recallStats(s, items, stored, 0.4f);
            std::printf("       after   0 new: accuracy %3.0f%%  own %.3f  best other %.3f\n", 100.0 * r.accuracy, r.own,
                        r.other);
            size_t added = 0;
            for (const auto& b : batches) {
                std::vector<std::string> more;
                for (char c : b) more.emplace_back(1, c);
                storeAll(s, more, kStore);
                added += b.size();
                r = recallStats(s, items, stored, 0.4f);
                std::printf("       after %3zu new: accuracy %3.0f%%  own %.3f  best other %.3f\n", added,
                            100.0 * r.accuracy, r.own, r.other);
            }
        }
    }
    return 0;
}

// CP6b retention under load (harsher than CP6): 8 memories are stored, then 16 more. Old
// memories must still all be recalled correctly from 40% cues, lose at most 0.05 of their
// margin (own minus best other, among the 8), and keep a gain of at least 0.05 over the
// untrained twin. With 8 items the last one's freshness weighs 1/8, not 1/3 as in CP6.
int runRetentionTest(const Config& cfg) {
    const std::string oldItems = "akzmqetw", newItems = "bdfghjlpnosuvxyc";
    std::printf("CP6b retention under load: 8 memories, then 16 more, recalled from 40%% cues\n");
    RecallStats before[2], after[2];
    for (int learning = 1; learning >= 0; --learning) {
        Session s(cfg, learning == 1);
        std::vector<std::string> list, more;
        for (char c : oldItems) list.emplace_back(1, c);
        for (char c : newItems) more.emplace_back(1, c);
        const Patterns stored = storeAll(s, list, kStore);
        before[learning] = recallStats(s, oldItems, stored, 0.4f);
        storeAll(s, more, kStore);
        after[learning] = recallStats(s, oldItems, stored, 0.4f);
    }
    const double marginBefore = before[1].own - before[1].other, marginAfter = after[1].own - after[1].other;
    const double drop = marginBefore - marginAfter;
    const double gainAfter = marginAfter - (after[0].own - after[0].other);
    std::printf("  learned:   before accuracy %3.0f%% margin %+.3f | after 16 new accuracy %3.0f%% margin %+.3f\n",
                100.0 * before[1].accuracy, marginBefore, 100.0 * after[1].accuracy, marginAfter);
    std::printf("  untrained: after accuracy %3.0f%% margin %+.3f\n", 100.0 * after[0].accuracy,
                after[0].own - after[0].other);
    const bool allRight = after[1].accuracy >= 0.999;
    const bool keeps = drop <= 0.05;
    const bool beats = gainAfter >= 0.05;
    std::printf("  all old memories recalled correctly: %s; margin lost %+.3f (need <= +0.050): %s; "
                "gain over untrained %+.3f (need >= +0.050): %s\n",
                allRight ? "yes" : "NO", drop, keeps ? "yes" : "NO", gainAfter, beats ? "yes" : "NO");
    return (allRight && keeps && beats) ? 0 : 3;
}

// Diagnostic: where activity sits. A letter is held (no learning); for each field, active voxels
// (strongest channel >= 0.1) are counted per depth layer x (x = 0 is the sensory face of the
// Input field), with the share of the field's volume that is active.
int runOccupancyTest(const Config& cfg, const std::string& item) {
    Session s(cfg, false);
    s.present(item, kStore, 1.0f, false, UINT64_MAX);
    const auto& v = s.matrix().voxelState();
    const uint32_t N = cfg.field_dim;
    const size_t perField = size_t(N) * N * N;
    std::printf("Occupancy: \"%s\" held %llu ticks, field side %u; active voxels per depth layer x\n", item.c_str(),
                (unsigned long long)kStore, N);
    for (uint32_t f = 0; f < kFields; ++f) {
        std::vector<size_t> perLayer(N, 0);
        size_t total = 0;
        for (size_t i = 0; i < perField; ++i) {
            const float* cell = v.data() + (f * perField + i) * C3;
            float strongest = 0.0f;
            for (uint32_t c = 0; c < C3; ++c) strongest = std::max(strongest, cell[c]);
            if (strongest >= 0.1f) {
                ++perLayer[i % N];
                ++total;
            }
        }
        size_t layers = 0;
        for (size_t n : perLayer) layers += n > 0;
        std::printf("  field %u: %4zu active (%.2f%% of volume), %2zu of %u layers used |", f, total,
                    100.0 * double(total) / double(perField), layers, N);
        for (size_t n : perLayer) std::printf(" %zu", n);
        std::printf("\n");
    }
    return 0;
}

// Diagnostic for streamed words: the learning signal while each word streams in, and recall
// over time (own similarity and best other, in 10-tick windows of the cue) for the learning
// matrix and its untrained twin, plus per-field own similarity over the recorded window.
int runStreamDiagTest(const Config& cfg) {
    const std::vector<std::string> items = {"apple ", "river ", "stone "};
    std::printf("Streamed-word diagnostic\n");
    std::vector<std::string> rows[2];
    for (int learning = 1; learning >= 0; --learning) {
        Session s(cfg, learning == 1);
        Patterns stored;
        for (const auto& item : items) {
            s.resetModulatorMean();
            stored.push_back(s.present(item, kStore, 1.0f, true, kStore / 2));
            if (learning == 1)
                std::printf("  storing \"%s\": mean learning signal %.3f\n", item.c_str(), s.meanModulator());
            s.silence(kGap, true);
        }
        const size_t perField = stored[0].size() / kFields;
        for (size_t k = 0; k < items.size(); ++k) {
            char line[256];
            int n = std::snprintf(line, sizeof(line), "    %-6s", items[k].c_str());
            std::vector<double> window, total;
            for (uint64_t t = 0; t < kCue; ++t) {
                auto chunk = s.present(items[k].substr(t % items[k].size(), 1), 1, kCueFraction, false, 0);
                if (window.size() != chunk.size()) window.assign(chunk.size(), 0.0);
                for (size_t i = 0; i < chunk.size(); ++i) window[i] += chunk[i];
                if (t >= kCue / 2) {
                    if (total.size() != chunk.size()) total.assign(chunk.size(), 0.0);
                    for (size_t i = 0; i < chunk.size(); ++i) total[i] += chunk[i];
                }
                if (t % 10 == 9) {
                    double own = lab::cosine(window, stored[k]), other = -1.0;
                    for (size_t j = 0; j < items.size(); ++j)
                        if (j != k) other = std::max(other, lab::cosine(window, stored[j]));
                    n += std::snprintf(line + n, sizeof(line) - size_t(n), " %.2f/%.2f", own, other);
                    std::fill(window.begin(), window.end(), 0.0);
                }
            }
            n += std::snprintf(line + n, sizeof(line) - size_t(n), " | fields");
            for (uint32_t f = 0; f < kFields; ++f)
                n += std::snprintf(line + n, sizeof(line) - size_t(n), " %.2f",
                                   lab::cosine(total, stored[k], f * perField, (f + 1) * perField));
            rows[learning].push_back(line);
            s.silence(kGap, false);
        }
    }
    for (int l = 1; l >= 0; --l) {
        std::printf("  %s: own/best-other per 10-tick window of the cue | own per field (recorded window)\n",
                    l ? "LEARNED" : "UNTRAINED");
        for (const auto& r : rows[l]) std::printf("%s\n", r.c_str());
    }
    return 0;
}

// Settling time (untrained): a letter is held for 150 ticks. The steady pattern is the mean 3D
// state over ticks 110-150. Settle = first tick from which the state's similarity to the
// steady pattern stays >= 0.9; per field too. Fade = ticks after the input stops until total
// activity falls below 5% of its steady level. Also reports how deep activity reaches.
int runSettleTest(const Config& cfg, const std::string& item) {
    Session s(cfg, false);
    const uint64_t on = 150, off = 150;
    std::vector<std::vector<double>> states;
    for (uint64_t t = 0; t < on; ++t) {
        std::vector<double> now;
        s.present(item, 1, 1.0f, false, 0);
        s.accumulate(now);
        states.push_back(now);
    }
    std::vector<double> steady(states[0].size(), 0.0);
    for (uint64_t t = 110; t < on; ++t)
        for (size_t i = 0; i < steady.size(); ++i) steady[i] += states[t][i];
    const size_t perField = steady.size() / kFields;
    auto settleTick = [&](size_t b, size_t e) {
        int64_t last = -1;
        for (uint64_t t = 0; t < on; ++t)
            if (lab::cosine(states[t], steady, b, e) < 0.9) last = int64_t(t);
        return last + 1;
    };
    double steadyTotal = 0.0;
    for (double x : steady) steadyTotal += x;
    steadyTotal /= 40.0;
    int64_t fade = -1;
    for (uint64_t t = 0; t < off; ++t) {
        std::vector<double> now;
        s.silence(1, false, 0);
        s.accumulate(now);
        double total = 0.0;
        for (double x : now) total += x;
        if (total < 0.05 * steadyTotal) {
            fade = int64_t(t) + 1;
            break;
        }
    }
    std::printf("Settling: \"%s\" held %llu ticks (untrained), field side %u\n", item.c_str(), (unsigned long long)on,
                cfg.field_dim);
    std::printf("  settle tick (similarity to steady >= 0.9 from then on): all %lld |", (long long)settleTick(0, steady.size()));
    for (uint32_t f = 0; f < kFields; ++f)
        std::printf(" field %u %lld", f, (long long)settleTick(f * perField, (f + 1) * perField));
    std::printf("\n  fade ticks after input stops (activity < 5%% of steady): %lld\n", (long long)fade);
    std::printf("  similarity to steady, every 5 ticks (all fields):");
    for (uint64_t t = 4; t < on; t += 5) std::printf(" %.2f", lab::cosine(states[t], steady));
    std::printf("\n  consecutive-tick similarity, last 20 ticks:");
    for (uint64_t t = on - 20; t < on; ++t) std::printf(" %.2f", lab::cosine(states[t], states[t - 1]));
    std::printf("\n");
    return 0;
}

// CP5b word completion (streamed language input): words are learned streamed letter by letter;
// then only the first `prefix` letters are heard once, followed by silence. The activity in
// the window after the prefix is compared with each word's stored (streamed) pattern: learning
// must make it resemble the cue's own word more than the other words, beyond an untrained twin.
int runCompletionTest(const Config& cfg, uint64_t prefix) {
    // Words with different endings: completing a word's tail must not look like another word
    // only because they end the same way ("apple"/"stone" share "e").
    const std::vector<std::string> items = {"apple ", "river ", "storm "};
    std::printf("CP5b word completion: learn 3 streamed words, hear the first %llu letters, read what follows\n",
                (unsigned long long)prefix);
    Specificity spec[2];
    std::vector<std::vector<double>> sims[2];
    for (int learning = 1; learning >= 0; --learning) {
        Session s(cfg, learning == 1);
        const Patterns stored = storeAll(s, items, kStore);
        Patterns after;
        for (const auto& item : items) {
            s.present(item.substr(0, prefix), prefix, 1.0f, false, UINT64_MAX);
            // Record the window after the prefix; every 5 ticks, how much activity remains and the
            // bin's own-word similarity minus the best other word (does the state move toward its
            // own word as the prefix's echo fades, or only fade?).
            std::vector<double> acc, bin;
            std::string trace, margins;
            const size_t own = size_t(&item - items.data());
            for (uint64_t t = 0; t < T(40); ++t) {
                std::vector<double> now;
                s.silence(1, false, 0);
                s.accumulate(now);
                if (acc.size() != now.size()) acc.assign(now.size(), 0.0);
                if (bin.size() != now.size()) bin.assign(now.size(), 0.0);
                double total = 0.0;
                for (size_t i = 0; i < now.size(); ++i) {
                    acc[i] += now[i];
                    bin[i] += now[i];
                    total += now[i];
                }
                if (t % 5 == 4) {
                    char buf[24];
                    std::snprintf(buf, sizeof(buf), " %.1f", total);
                    trace += buf;
                    double best = -1.0;
                    for (size_t k = 0; k < stored.size(); ++k)
                        if (k != own) best = std::max(best, lab::cosine(bin, stored[k], 0, bin.size()));
                    std::snprintf(buf, sizeof(buf), " %+.2f", lab::cosine(bin, stored[own], 0, bin.size()) - best);
                    margins += buf;
                    bin.assign(bin.size(), 0.0);
                }
            }
            std::printf("    %s \"%s\": activity after the prefix, every 5 ticks:%s\n", learning ? "learned  " : "untrained",
                        item.substr(0, prefix).c_str(), trace.c_str());
            std::printf("      own-word margin per 5 ticks:%s\n", margins.c_str());
            after.push_back(acc);
            s.silence(kGap, false);
        }
        sims[learning] = lab::similarityMatrix(after, stored);
        spec[learning] = lab::specificity(sims[learning]);
    }
    for (int l = 1; l >= 0; --l) {
        std::printf("  %s: after-prefix activity vs stored words (apple river storm)\n", l ? "learned" : "untrained");
        for (size_t k = 0; k < items.size(); ++k)
            std::printf("    %-6s %.3f %.3f %.3f\n", items[k].substr(0, prefix).c_str(), sims[l][k][0], sims[l][k][1],
                        sims[l][k][2]);
    }
    const double gain = spec[1].margin - spec[0].margin;
    const bool pass = spec[1].identifies && gain >= 0.05;
    std::printf("  word completion: learned margin %+.3f (identifies all: %s) vs untrained %+.3f -> gain %+.3f "
                "(need >= +0.050): %s\n", spec[1].margin, spec[1].identifies ? "yes" : "NO", spec[0].margin, gain,
                pass ? "PASS" : "FAIL");
    return pass ? 0 : 3;
}

// Memory health (diagnostic): which subsystem misbehaves when memories blur. 8 memories are
// stored as in CP3, on a learning matrix and an untrained twin, then each is cued (40%).
//   STORE    per field: overlap between stored patterns (mean pairwise similarity) and density.
//   WEIGHTS  per field: fill of the plastic budget, share of channels full / used, and the share
//            of outgoing learned strength held by the top 1% of source channels (hubs).
//   RECALL   per field, at points in the cue: learned share of the input of firing voxels,
//            activity relative to the stored pattern's level, own-memory margin, field gain.
//   ENDS IN  which memory each cue's activity resembles most at the end of the cue.
//   AFTER    total activity per field in the silence after the cue (runaway or fading).
int runHealthTest(const Config& cfg) {
    const std::vector<std::string> items = {"a", "k", "z", "m", "q", "e", "t", "w"};
    const char* names[kFields] = {"Input", "Memory", "Reasoning", "Output"};
    const uint64_t points[] = {T(5), T(15), T(30), kCue - 1};
    std::printf("Memory health: %zu memories stored, each cued at %.0f%%\n", items.size(), 100.0 * kCueFraction);
    for (int learning = 1; learning >= 0; --learning) {
        Session s(cfg, learning == 1);
        const Patterns stored = storeAll(s, items, kStore);
        const size_t perField = stored[0].size() / kFields;
        const double storedTicks = double(kStore - kStore / 2);
        std::printf("  %s\n", learning ? "LEARNED" : "UNTRAINED");

        std::printf("    STORE    field      overlap  density\n");
        for (uint32_t f = 0; f < kFields; ++f) {
            const size_t b = f * perField, e = (f + 1) * perField;
            double overlap = 0.0, density = 0.0;
            int n = 0;
            for (size_t i = 0; i < stored.size(); ++i) {
                size_t on = 0;
                for (size_t k = b; k < e; ++k) on += stored[i][k] > 0.0;
                density += double(on) / double(perField) / double(stored.size());
                for (size_t j = i + 1; j < stored.size(); ++j) {
                    overlap += lab::cosine(stored[i], stored[j], b, e);
                    ++n;
                }
            }
            std::printf("             %-10s %.3f   %.3f\n", names[f], overlap / n, density);
        }

        if (learning) {
            // weightHealth() only reads the weights; the matrix is accessed through the session.
            const auto health = const_cast<NeuralCellularMatrix&>(s.matrix()).weightHealth();
            std::printf("    WEIGHTS  field      fill   full   used   top1%%-sources\n");
            for (uint32_t f = 0; f < kFields; ++f)
                std::printf("             %-10s %.3f  %.3f  %.3f  %.3f\n", names[f], health[f].meanFill, health[f].shareFull,
                            health[f].shareUsed, health[f].topSourceShare);
        }

        // Per point in the cue and field: learned share, activity ratio, margin, gain (summed over cues).
        const size_t P = sizeof(points) / sizeof(points[0]);
        std::vector<std::array<double, 4>> acc(P * kFields, std::array<double, 4>{0, 0, 0, 0});
        std::vector<size_t> endsIn(items.size());
        std::vector<std::array<double, kFields>> after(8, std::array<double, kFields>{});
        for (size_t k = 0; k < items.size(); ++k) {
            std::vector<double> window, bin;
            size_t pi = 0;
            for (uint64_t t = 0; t < kCue; ++t) {
                s.present(items[k], 1, kCueFraction, false, UINT64_MAX);
                s.accumulate(bin);
                if (t >= kCue / 2) s.accumulate(window);
                if (pi < P && t == points[pi]) {
                    const auto drive = s.matrix().driveBreakdown();
                    const auto& gains = s.matrix().fieldGains();
                    for (uint32_t f = 0; f < kFields; ++f) {
                        const size_t b = f * perField, e = (f + 1) * perField;
                        double level = 0.0, storedLevel = 0.0;
                        for (size_t i = b; i < e; ++i) {
                            level += bin[i];
                            storedLevel += stored[k][i];
                        }
                        const uint64_t binTicks = pi == 0 ? points[0] + 1 : points[pi] - points[pi - 1];
                        double best = -1.0;
                        for (size_t j = 0; j < stored.size(); ++j)
                            if (j != k) best = std::max(best, lab::cosine(bin, stored[j], b, e));
                        auto& a = acc[pi * kFields + f];
                        a[0] += drive[f].totalActive > 0.0 ? drive[f].plasticActive / drive[f].totalActive : 0.0;
                        a[1] += storedLevel > 0.0 ? (level / double(binTicks)) / (storedLevel / storedTicks) : 0.0;
                        a[2] += lab::cosine(bin, stored[k], b, e) - best;
                        a[3] += gains[f];
                    }
                    bin.assign(bin.size(), 0.0);
                    ++pi;
                }
            }
            double best = -1.0;
            for (size_t j = 0; j < stored.size(); ++j) {
                const double sim = lab::cosine(window, stored[j]);
                if (sim > best) {
                    best = sim;
                    endsIn[k] = j;
                }
            }
            for (int b = 0; b < 8; ++b) {
                std::vector<double> now;
                s.silence(T(5), false, T(5) - 1);
                s.accumulate(now);
                for (uint32_t f = 0; f < kFields; ++f)
                    for (size_t i = f * perField; i < (f + 1) * perField; ++i) after[b][f] += now[i] / double(items.size());
            }
            s.silence(kGap > T(40) ? kGap - T(40) : 0, false);
        }
        std::printf("    RECALL   tick  field      learned-share  activity/stored  own-margin  gain\n");
        for (size_t pi = 0; pi < P; ++pi)
            for (uint32_t f = 0; f < kFields; ++f) {
                const auto& a = acc[pi * kFields + f];
                const double n = double(items.size());
                std::printf("             %4llu  %-10s %.3f          %.2f             %+.3f      %.2f\n",
                            (unsigned long long)points[pi], names[f], a[0] / n, a[1] / n, a[2] / n, a[3] / n);
            }
        {
            const auto wi = s.matrix().meanInhibitionWeight();
            const auto& st = s.matrix().voxelState();
            std::printf("    INHIB    learned inhibition weight per field:");
            for (uint32_t f = 0; f < kFields; ++f) std::printf(" %s %.3f", names[f], wi[f]);
            double ySum = 0.0;
            size_t yN = 0;
            for (size_t v = 0; v < st.size() / C3; ++v) {
                double y = 0.0;
                for (uint32_t c = 0; c < C3; ++c) y += st[v * C3 + c];
                if (y > 0.0) {
                    ySum += y / C3;
                    ++yN;
                }
            }
            std::printf(" | mean channel output of firing voxels now %.3f\n", yN ? ySum / double(yN) : 0.0);
        }
        std::printf("    ENDS IN  ");
        for (size_t k = 0; k < items.size(); ++k)
            std::printf("%s->%s%s ", items[k].c_str(), items[endsIn[k]].c_str(), endsIn[k] == k ? "" : "(!)");
        std::printf("\n    AFTER    activity per field every 5 ticks of silence after the cue:\n");
        for (uint32_t f = 0; f < kFields; ++f) {
            std::printf("             %-10s", names[f]);
            for (int b = 0; b < 8; ++b) std::printf(" %7.1f", after[b][f]);
            std::printf("\n");
        }
    }
    return 0;
}

// Hum (diagnostic): where the activity that lingers after recall comes from. 8 memories are
// stored as in CP3, then each is cued (40%) and followed through 60 ticks of silence. Every 5
// ticks, per field: firing voxels, active sheet cells and line cells, and the net input of the
// firing voxels by source (learned within field / learned 4D / fixed local (self + neighbours)
// / fixed long-range / fixed 4D link / input depth / fixed spread projection / upward from the
// voxel's own sheet), averaged per firing voxel; plus mode, gains and transmitter resource.
int runHumTest(const Config& cfg) {
    const std::vector<std::string> items = {"a", "k", "z", "m", "q", "e", "t", "w"};
    const char* names[kFields] = {"Input", "Memory", "Reasoning", "Output"};
    const int bins = int(T(60) / T(5));
    std::printf("Hum: activity after recall, by source (sums over %zu cues)\n", items.size());
    for (int learning = 1; learning >= 0; --learning) {
        Session s(cfg, learning == 1);
        storeAll(s, items, kStore);
        const auto& m = s.matrix();
        const size_t V = m.voxelState().size() / C3, Vf = V / kFields;
        const size_t sheetPerField = m.sheetState().size() / kFields, linePerField = m.lineState().size() / kFields;
        // [bin][field]: firing, sheet cells, line cells, sources...
        std::vector<std::array<std::array<double, 3 + kSources>, kFields>> acc(bins);
        std::vector<double> resource(bins, 0.0), mod(bins, 0.0);
        std::vector<std::array<double, kFields>> gain(bins);
        for (const auto& item : items) {
            s.present(item, kCue, kCueFraction, false, UINT64_MAX);
            for (int b = 0; b < bins; ++b) {
                s.silence(T(5), false);
                const auto src = m.driveSources();
                const auto& g = m.fieldGains();
                for (uint32_t f = 0; f < kFields; ++f) {
                    auto& a = acc[b][f];
                    a[0] += src[f].firing;
                    size_t sheets = 0, lines = 0;
                    for (size_t i = f * sheetPerField; i < (f + 1) * sheetPerField; ++i) sheets += m.sheetState()[i] > 0.0f;
                    for (size_t i = f * linePerField; i < (f + 1) * linePerField; ++i) lines += m.lineState()[i] > 0.0f;
                    a[1] += double(sheets);
                    a[2] += double(lines);
                    for (uint32_t k = 0; k < kSources; ++k) a[3 + k] += src[f].net[k];
                    gain[b][f] += g[f] / double(items.size());
                }
                resource[b] += m.meanResource() / double(items.size());
                mod[b] += s.lastMode() / double(items.size());
            }
            s.silence(kGap, false);
        }
        (void)Vf;
        std::printf("  %s\n", learning ? "LEARNED" : "UNTRAINED");
        std::printf("    tick field      firing  sheet-ch  line-ch | per firing voxel: learnW  learn4D  local  longR  fix4D  depth  spread  upward  inhib | gain\n");
        for (int b = 0; b < bins; ++b) {
            for (uint32_t f = 0; f < kFields; ++f) {
                const auto& a = acc[b][f];
                const double n = double(items.size()), fv = std::max(1.0, a[0]);
                std::printf("    %4d %-10s %6.1f  %8.1f  %7.1f |                  ", int((b + 1) * T(5)), names[f], a[0] / n,
                            a[1] / n, a[2] / n);
                for (uint32_t k = 0; k < kSources; ++k) std::printf(" %6.3f", a[3 + k] / fv);
                std::printf(" | %.2f\n", gain[b][f]);
            }
            std::printf("         mode (1 = memories suppressed) %.2f, transmitter resource %.3f\n", mod[b], resource[b]);
        }
    }
    return 0;
}

// Chain (diagnostic): can a learned word carry itself forward? After the CP5b words are learned,
// each word is heard again (no learning) and the 3D state is summed per letter position. The
// learned flow (Matrix::plasticFlow) between those states shows, per word:
//   next  = position i -> i+1 of the same word (the chain completion needs),
//   back  = i+1 -> i, self = i -> i, other = best flow from i into any position of another word.
// Also: the learning signal during storage, how alike the letter states of different words are,
// and each field's share of the chain flow.
int runChainTest(const Config& cfg) {
    const std::vector<std::string> items = {"apple ", "river ", "storm "};
    Session s(cfg, true);
    s.resetModulatorMean();
    storeAll(s, items, kStore);
    std::printf("Chain: learning signal during storage %.3f, total learned strength %.1f\n", s.meanModulator(),
                s.matrix().totalPlasticStrength());
    std::vector<std::vector<std::vector<double>>> P(items.size());
    for (size_t w = 0; w < items.size(); ++w) {
        const std::string& word = items[w];
        P[w].assign(word.size(), {});
        for (uint64_t t = 0; t < kStore; ++t) {
            s.present(std::string(1, word[t % word.size()]), 1, 1.0f, false, UINT64_MAX);
            if (t >= word.size()) s.accumulate(P[w][t % word.size()]);
        }
        s.silence(kGap, false);
    }
    auto& m = const_cast<NeuralCellularMatrix&>(s.matrix());
    double sumNext = 0.0, sumOther = 0.0, simOwn = 0.0, simOther = 0.0;
    int n = 0, no = 0;
    for (size_t w = 0; w < items.size(); ++w) {
        const std::string& word = items[w];
        std::printf("  %-7s  pos  next    back    self    other(best) | similarity next-letter / other-word\n",
                    ("\"" + word.substr(0, word.size() - 1) + "\"").c_str());
        for (size_t i = 0; i < word.size(); ++i) {
            const size_t j = (i + 1) % word.size();
            const double next = m.plasticFlow(P[w][i], P[w][j]);
            const double back = m.plasticFlow(P[w][j], P[w][i]);
            const double self = m.plasticFlow(P[w][i], P[w][i]);
            double other = 0.0, sim = 0.0;
            int ns = 0;
            for (size_t v = 0; v < items.size(); ++v)
                if (v != w)
                    for (size_t k = 0; k < items[v].size(); ++k) {
                        other = std::max(other, m.plasticFlow(P[w][i], P[v][k]));
                        sim += lab::cosine(P[w][i], P[v][k]);
                        ++ns;
                    }
            const double simNext = lab::cosine(P[w][i], P[w][j]);
            std::printf("           %c->%c %.4f  %.4f  %.4f  %.4f      | %.3f / %.3f\n", word[i] == ' ' ? '_' : word[i],
                        word[j] == ' ' ? '_' : word[j], next, back, self, other, simNext, sim / ns);
            sumNext += next;
            sumOther += other;
            simOwn += simNext;
            simOther += sim / ns;
            ++n;
            ++no;
        }
    }
    std::printf("  mean: next %.4f vs best other %.4f (ratio %.2f); letter-state similarity next %.3f vs other words %.3f\n",
                sumNext / n, sumOther / n, sumOther > 0 ? sumNext / sumOther : 0.0, simOwn / n, simOther / no);
    // Per field: flow of the chain restricted to each target field.
    const size_t perField = P[0][0].size() / kFields;
    std::printf("  other-word letter-state similarity by field:");
    for (uint32_t f = 0; f < kFields; ++f) {
        double sim = 0.0;
        int ns = 0;
        for (size_t w = 0; w < items.size(); ++w)
            for (size_t v = w + 1; v < items.size(); ++v)
                for (size_t i = 0; i < items[w].size(); ++i)
                    for (size_t k = 0; k < items[v].size(); ++k) {
                        sim += lab::cosine(P[w][i], P[v][k], f * perField, (f + 1) * perField);
                        ++ns;
                    }
        std::printf(" %s %.3f", f == 0 ? "Input" : f == 1 ? "Memory" : f == 2 ? "Reasoning" : "Output", sim / ns);
    }
    std::printf("\n  single letters (held, untrained twin) similarity by field, words' letters pooled:");
    {
        Session u(cfg, false);
        std::string letters = "aplerivstom ";
        std::vector<std::vector<double>> L;
        for (char c : letters) {
            L.push_back(u.present(std::string(1, c), kStore, 1.0f, false, kStore / 2));
            u.silence(kGap, false);
        }
        for (uint32_t f = 0; f < kFields; ++f) {
            double sim = 0.0;
            int ns = 0;
            for (size_t i = 0; i < L.size(); ++i)
                for (size_t k = i + 1; k < L.size(); ++k) {
                    sim += lab::cosine(L[i], L[k], f * perField, (f + 1) * perField);
                    ++ns;
                }
            std::printf(" %s %.3f", f == 0 ? "Input" : f == 1 ? "Memory" : f == 2 ? "Reasoning" : "Output", sim / ns);
        }
        // Fingerprint overlap of the letters themselves (the sensory code).
        double fo = 0.0;
        int nf = 0;
        for (size_t i = 0; i < letters.size(); ++i)
            for (size_t k = i + 1; k < letters.size(); ++k) {
                const auto& A = u.codebook().fingerprint(char32_t(uint8_t(letters[i])));
                const auto& B = u.codebook().fingerprint(char32_t(uint8_t(letters[k])));
                size_t shared = 0;
                for (uint32_t x : A) shared += std::count(B.begin(), B.end(), x);
                fo += double(shared) / double(std::max<size_t>(1, A.size()));
                ++nf;
            }
        std::printf("\n  fingerprint overlap between letters (share of lines shared): %.3f", fo / nf);
    }
    std::printf("\n");
    std::printf("  chain flow by target field:");
    const char* names[kFields] = {"Input", "Memory", "Reasoning", "Output"};
    for (uint32_t f = 0; f < kFields; ++f) {
        double fl = 0.0;
        for (size_t w = 0; w < items.size(); ++w)
            for (size_t i = 0; i < items[w].size(); ++i) {
                std::vector<double> to = P[w][(i + 1) % items[w].size()];
                for (size_t k = 0; k < to.size(); ++k)
                    if (k / perField != f) to[k] = 0.0;
                fl += m.plasticFlow(P[w][i], to);
            }
        std::printf(" %s %.4f", names[f], fl / n);
    }
    std::printf("\n");
    return 0;
}

// Overlap (diagnostic): share of sensory lines each pair of letters' fingerprints have in common,
// and how alike the letters' settled 3D patterns are per field (untrained).
int runOverlapTest(const Config& cfg) {
    const std::string letters = "akzmqe";
    Session u(cfg, false);
    std::printf("Fingerprint overlap (share of lines shared) and untrained pattern similarity (Input/Memory/Output)\n       ");
    for (char c : letters) std::printf("        %c         ", c);
    std::printf("\n");
    std::vector<std::vector<double>> P;
    for (char c : letters) {
        P.push_back(u.present(std::string(1, c), kStore, 1.0f, false, kStore / 2));
        u.silence(kGap, false);
    }
    const size_t perField = P[0].size() / kFields;
    for (size_t i = 0; i < letters.size(); ++i) {
        std::printf("  %c  ", letters[i]);
        for (size_t k = 0; k < letters.size(); ++k) {
            const auto& A = u.codebook().fingerprint(char32_t(uint8_t(letters[i])));
            const auto& B = u.codebook().fingerprint(char32_t(uint8_t(letters[k])));
            size_t shared = 0;
            for (uint32_t x : A) shared += std::count(B.begin(), B.end(), x);
            std::printf(" %.2f/%.2f/%.2f/%.2f", double(shared) / double(std::max<size_t>(1, A.size())),
                        lab::cosine(P[i], P[k], 0, perField), lab::cosine(P[i], P[k], perField, 2 * perField),
                        lab::cosine(P[i], P[k], 3 * perField, 4 * perField));
        }
        std::printf("\n");
    }
    return 0;
}

// Interference (diagnostic): the CP6 sequence (store a k z, recall them, store m q e), then each
// old item is cued (40%) and followed every 5 ticks: similarity of the recent state to its own
// memory and to each new memory (all fields), plus per field at the end of the cue. Also the
// learned inhibition and the learned self-links of each stored memory.
int runInterferenceTest(const Config& cfg) {
    const std::vector<std::string> oldItems = {"a", "k", "z"}, newItems = {"m", "q", "e"};
    Session s(cfg, true);
    Patterns stored = storeAll(s, oldItems, kStore);
    recallAll(s, oldItems);
    const Patterns storedNew = storeAll(s, newItems, kStore);
    stored.insert(stored.end(), storedNew.begin(), storedNew.end());
    const size_t perField = stored[0].size() / kFields;
    std::printf("Interference: learned self-links a k z m q e:");
    for (const auto& p : stored) std::printf(" %.3f", s.flow(p, p));
    std::printf("\n  stored pattern size (sum of activity) a k z m q e:");
    for (const auto& p : stored) {
        double t = 0.0;
        for (double x : p) t += x;
        std::printf(" %.0f", t);
    }
    std::printf("\n");
    for (size_t k = 0; k < oldItems.size(); ++k) {
        std::printf("  cue %s: every 5 ticks, own / m / q / e\n   ", oldItems[k].c_str());
        std::vector<double> bin, window;
        for (uint64_t t = 0; t < kCue; ++t) {
            s.present(oldItems[k], 1, kCueFraction, false, UINT64_MAX);
            s.accumulate(bin);
            if (t >= kCue / 2) s.accumulate(window);
            if (t % T(5) == T(5) - 1) {
                std::printf(" [%.2f %.2f %.2f %.2f]", lab::cosine(bin, stored[k]), lab::cosine(bin, stored[3]),
                            lab::cosine(bin, stored[4]), lab::cosine(bin, stored[5]));
                bin.assign(bin.size(), 0.0);
            }
        }
        std::printf("\n    per field at the end (own/best new):");
        for (uint32_t f = 0; f < kFields; ++f) {
            double best = 0.0;
            for (size_t j = 3; j < 6; ++j) best = std::max(best, lab::cosine(window, stored[j], f * perField, (f + 1) * perField));
            std::printf(" %.2f/%.2f", lab::cosine(window, stored[k], f * perField, (f + 1) * perField), best);
        }
        std::printf("\n");
        s.silence(kGap, false);
    }
    return 0;
}

// Discrimination (diagnostic): how much of a small input difference is still visible at each
// level. The letter 'a' and copies of it with k sensory lines replaced are each held (untrained,
// identical wiring); the settled activity is compared with the original's at the line (1D), sheet
// (2D) and voxel (3D) level of every field. "seen" = 1 - similarity: 0 = looks identical,
// 1 = looks unrelated. The input difference is the share of sensory lines that changed.
int runDiscriminationTest(const Config& cfg) {
    const char* names[kFields] = {"Input", "Memory", "Reasoning", "Output"};
    struct Levels {
        std::vector<double> line, sheet, voxel;
    };
    auto add = [](std::vector<double>& acc, const AVec<float>& st) {
        if (acc.size() != st.size()) acc.assign(st.size(), 0.0);
        for (size_t i = 0; i < st.size(); ++i) acc[i] += st[i];
    };
    auto settle = [&](const std::vector<uint32_t>& fp) {
        Session s(cfg, false);
        Levels l;
        for (uint64_t t = 0; t < kStore; ++t) {
            s.tick(&fp, &fp, false);
            if (t >= kStore / 2) {
                add(l.line, s.matrix().lineState());
                add(l.sheet, s.matrix().sheetState());
                add(l.voxel, s.matrix().voxelState());
            }
        }
        return l;
    };
    Session probe(cfg, false);
    const std::vector<uint32_t> base = probe.codebook().fingerprint(U'a');
    const uint32_t surface = cfg.surfaceLines();
    const Levels ref = settle(base);
    std::printf("Discrimination: 'a' has %zu sensory lines of %u; difference seen per level (0 = identical, 1 = unrelated)\n",
                base.size(), surface);
    std::printf("  changed  input   | level   Input  Memory Reason Output\n");
    std::vector<size_t> ks = {1, 2, 4, base.size() / 4, base.size() / 2, base.size()};
    for (size_t k : ks) {
        if (k == 0 || k > base.size()) continue;
        // Replace the first k lines by unused surface lines (deterministic).
        std::vector<uint32_t> fp = base;
        uint32_t candidate = 0;
        for (size_t i = 0; i < k; ++i) {
            while (std::find(base.begin(), base.end(), candidate) != base.end() ||
                   std::find(fp.begin(), fp.end(), candidate) != fp.end())
                candidate = (candidate + 7) % surface;
            fp[i] = candidate;
        }
        const Levels v = settle(fp);
        const struct {
            const char* name;
            const std::vector<double>* a;
            const std::vector<double>* b;
        } rows[3] = {{"lines ", &ref.line, &v.line}, {"sheets", &ref.sheet, &v.sheet}, {"voxels", &ref.voxel, &v.voxel}};
        for (int r = 0; r < 3; ++r) {
            if (r == 0) std::printf("  %4zu     %.3f   | ", k, double(k) / double(base.size()));
            else std::printf("                   | ");
            std::printf("%s ", rows[r].name);
            const size_t per = rows[r].a->size() / kFields;
            for (uint32_t f = 0; f < kFields; ++f)
                std::printf(" %.3f ", 1.0 - lab::cosine(*rows[r].a, *rows[r].b, f * per, (f + 1) * per));
            std::printf("\n");
        }
    }
    (void)names;
    return 0;
}

} // namespace ncm
