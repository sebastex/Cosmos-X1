// Stage 1 memory suite (spec Section 10): the checks that make Stage 1 a foundation for
// Stage 2. Every test runs the same protocol on a learning matrix and on an identical
// matrix that never learns, and passes only if learning makes recall measurably more
// specific. Bars are fixed in advance.

#include <cmath>
#include <numeric>
#include <algorithm>
#include <cstdio>
#include <cstdlib>
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
    // Word memory (2026-10-01): the honest pair test (8 word pairs, true recall of the partner).
    // The earlier streamed-word (CP5) and completion (CP5b) checks could be passed by the echo of
    // the cue alone (an untrained matrix scored as well), so they are diagnostics now.
    std::printf("\n=== CP5w ===\n");
    results.push_back({"CP5w word pairs (8)", runPairLoadTest(cfg, 8)});
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

namespace {

// Hears the first `prefix` letters of each word once, then records what follows in silence.
Patterns completeAll(Session& s, const std::vector<std::string>& items, uint64_t prefix) {
    Patterns after;
    for (const auto& item : items) {
        s.present(item.substr(0, prefix), prefix, 1.0f, false, UINT64_MAX);
        after.push_back(s.silence(T(40), false, 0));
        s.silence(kGap, false);
    }
    return after;
}

} // namespace

// Word capacity (word version of CP3): 8 words are learned streamed letter by letter; each is
// then recalled from its first 3 letters (heard once), read in the silence that follows.
int runWordCapacityTest(const Config& cfg) {
    const std::vector<std::string> items = {"apple ", "river ", "storm ", "candy ", "light ", "mouse ", "bench ", "think "};
    std::printf("Word capacity: %zu streamed words, each completed from its first 3 letters\n", items.size());
    Comparison c;
    for (int learning = 1; learning >= 0; --learning) {
        Session s(cfg, learning == 1);
        const Patterns stored = storeAll(s, items, kStore);
        // Diagnostic: how alike the stored word shapes are (per field), and how full the link budget is.
        const size_t perField = stored[0].size() / kFields;
        std::printf("  %s: word shapes alike (Input Memory Reasoning Output):", learning ? "learned  " : "untrained");
        for (uint32_t f = 0; f < kFields; ++f) {
            double sim = 0.0;
            int n = 0;
            for (size_t i = 0; i < stored.size(); ++i)
                for (size_t j = i + 1; j < stored.size(); ++j, ++n)
                    sim += lab::cosine(stored[i], stored[j], f * perField, (f + 1) * perField);
            std::printf(" %.3f", sim / n);
        }
        if (learning) {
            const auto h = const_cast<NeuralCellularMatrix&>(s.matrix()).weightHealth();
            std::printf(" | link budget used (mean fill):");
            for (uint32_t f = 0; f < kFields; ++f) std::printf(" %.3f", h[f].meanFill);
            std::printf(" full:");
            for (uint32_t f = 0; f < kFields; ++f) std::printf(" %.3f", h[f].shareFull);
        }
        std::printf("\n");
        (learning ? c.learned : c.untrained) = lab::specificity(lab::similarityMatrix(completeAll(s, items, 3), stored));
    }
    printComparison("8 words:", c);
    return c.pass() ? 0 : 3;
}

// Word continual learning (word version of CP6): learn 3 words, complete them, learn 3 more,
// complete all 6. Same pass rule as CP6: old and new words completed beyond the untrained
// twin, and the old words' margin drops by at most 0.05.
int runWordContinualTest(const Config& cfg, bool swapped) {
    std::vector<std::string> oldItems = {"apple ", "river ", "storm "}, newItems = {"candy ", "light ", "mouse "};
    if (swapped) std::swap(oldItems, newItems); // diagnostic: is a weak second batch due to order or to the words?
    std::vector<std::string> all = oldItems;
    all.insert(all.end(), newItems.begin(), newItems.end());
    std::printf("Word continual learning: learn 3 words, complete them, learn 3 more, complete all 6\n");
    Comparison before, afterOld, afterNew;
    for (int learning = 1; learning >= 0; --learning) {
        Session s(cfg, learning == 1);
        Patterns stored = storeAll(s, oldItems, kStore);
        const auto simBefore = lab::similarityMatrix(completeAll(s, oldItems, 3), stored);
        const Patterns storedNew = storeAll(s, newItems, kStore);
        stored.insert(stored.end(), storedNew.begin(), storedNew.end());
        const auto sim = lab::similarityMatrix(completeAll(s, all, 3), stored);
        (learning ? before.learned : before.untrained) = lab::specificity(simBefore);
        (learning ? afterOld.learned : afterOld.untrained) = lab::specificity(sim, {0, 1, 2});
        (learning ? afterNew.learned : afterNew.untrained) = lab::specificity(sim, {3, 4, 5});
    }
    printComparison("old words, before new learning:", before);
    printComparison("old words, after new learning:", afterOld);
    printComparison("new words:", afterNew);
    const double forgetting = before.learned.margin - afterOld.learned.margin;
    const bool keeps = forgetting <= 0.05;
    std::printf("  forgetting of old words: %+.3f (need <= +0.050): %s\n", forgetting, keeps ? "PASS" : "FAIL");
    return (before.pass() && afterOld.pass() && afterNew.pass() && keeps) ? 0 : 3;
}

// Word shapes (diagnostic): why streamed words look alike. 8 words are streamed (untrained);
// for every pair: shared letters and similarity of the 3D shapes. Then the same with the
// common background removed (the mean shape of all words subtracted), and how much of each
// word's activity lies in cells that are active for most words.
int runWordShapeTest(const Config& cfg, bool withSpace, uint64_t hold) {
    hold = std::max<uint64_t>(1, hold); // ticks each letter is held in the steadiness part (1 = normal stream)
    std::vector<std::string> items = {"apple ", "river ", "storm ", "candy ", "light ", "mouse ", "bench ", "think "};
    if (!withSpace)
        for (auto& w : items) w.pop_back();
    Session s(cfg, false);
    const Patterns P = storeAll(s, items, kStore);
    const size_t n = P.size(), dim = P[0].size();
    std::vector<double> mean(dim, 0.0);
    for (const auto& p : P)
        for (size_t i = 0; i < dim; ++i) mean[i] += p[i] / double(n);
    Patterns C = P;
    for (auto& c : C)
        for (size_t i = 0; i < dim; ++i) c[i] -= mean[i];
    std::printf("Word shapes (untrained): pair, shared letters (not counting the space), similarity raw -> background removed\n");
    double byShared[6] = {}, bySharedC[6] = {};
    int cnt[6] = {};
    for (size_t i = 0; i < n; ++i)
        for (size_t j = i + 1; j < n; ++j) {
            int shared = 0;
            for (char ch = 'a'; ch <= 'z'; ++ch)
                shared += items[i].find(ch) != std::string::npos && items[j].find(ch) != std::string::npos;
            const double raw = lab::cosine(P[i], P[j]), cen = lab::cosine(C[i], C[j]);
            byShared[std::min(shared, 5)] += raw;
            bySharedC[std::min(shared, 5)] += cen;
            ++cnt[std::min(shared, 5)];
        }
    for (int k = 0; k < 6; ++k)
        if (cnt[k])
            std::printf("  %d shared letters (%2d pairs): alike %.3f -> %+.3f with background removed\n", k, cnt[k],
                        byShared[k] / cnt[k], bySharedC[k] / cnt[k]);
    // Share of activity in cells active for at least 6 of the 8 words, and number of such cells.
    size_t common = 0, any = 0;
    double actCommon = 0.0, actAll = 0.0;
    for (size_t i = 0; i < dim; ++i) {
        int in = 0;
        double a = 0.0;
        for (const auto& p : P) {
            in += p[i] > 0.0;
            a += p[i];
        }
        any += in > 0;
        actAll += a;
        if (in >= 6) {
            ++common;
            actCommon += a;
        }
    }
    std::printf("  cells active for >= 6 of 8 words: %.1f%% of all active cells, carrying %.1f%% of the activity\n",
                100.0 * double(common) / double(std::max<size_t>(1, any)), 100.0 * actCommon / std::max(1e-12, actAll));
    // Share of cells a word's shape uses, against a single held letter's and one moment of a word.
    {
        double wordDensity = 0.0;
        for (const auto& p : P) {
            size_t on = 0;
            for (double x : p) on += x > 0.0;
            wordDensity += double(on) / double(dim) / double(n);
        }
        Session u(cfg, false);
        const std::vector<double> letter = u.present("a", kStore, 1.0f, false, kStore / 2);
        size_t onL = 0;
        for (double x : letter) onL += x > 0.0;
        u.silence(kGap, false);
        u.present("apple ", kStore - 1, 1.0f, false, UINT64_MAX);
        const std::vector<double> moment = u.present("apple ", 1, 1.0f, false, 0);
        size_t onM = 0;
        for (double x : moment) onM += x > 0.0;
        std::printf("  share of cells used: a whole word's shape %.1f%%, one moment of a word %.1f%%, a held letter %.1f%%\n",
                    100.0 * wordDensity, 100.0 * double(onM) / double(dim), 100.0 * double(onL) / double(dim));
    }
    // Per field: cells used by one moment and by the whole word, and how alike the 3D state is
    // from one update to the next and between moments half a word apart (steadiness in time).
    {
        Session u(cfg, false);
        const std::string word = items[0];
        const uint64_t period = word.size() * hold;
        for (uint64_t t = 0; t < 10 * period; ++t) u.present(std::string(1, word[(t / hold) % word.size()]), 1, 1.0f, false, UINT64_MAX);
        std::vector<std::vector<double>> moments;
        for (uint64_t t = 0; t < 10 * period; ++t)
            moments.push_back(u.present(std::string(1, word[(t / hold) % word.size()]), 1, 1.0f, false, 0));
        // Whole-cycle shapes: the summed state over each repeat of the word; how alike are repeats?
        {
            std::vector<std::vector<double>> cycles(10);
            for (uint64_t t = 0; t < 10 * period; ++t) {
                auto& c = cycles[t / period];
                if (c.size() != moments[t].size()) c.assign(moments[t].size(), 0.0);
                for (size_t i = 0; i < c.size(); ++i) c[i] += moments[t][i];
            }
            double rep = 0.0;
            for (size_t c = 0; c + 1 < cycles.size(); ++c) rep += lab::cosine(cycles[c], cycles[c + 1]) / double(cycles.size() - 1);
            std::printf("  each letter held %llu tick(s): one repeat of the word vs the next repeat: %.3f alike\n",
                        (unsigned long long)hold, rep);
        }
        const size_t per = dim / kFields;
        std::printf("  per field (Input Memory Reasoning Output), word %s:\n", word.c_str());
        const char* label[4] = {"cells in one moment %", "cells over the word % ", "alike 3 ticks apart   ", "alike 12 ticks apart  "};
        for (int row = 0; row < 4; ++row) {
            std::printf("    %s", label[row]);
            for (uint32_t f = 0; f < kFields; ++f) {
                double v = 0.0;
                if (row == 0) {
                    for (const auto& m : moments) {
                        size_t on = 0;
                        for (size_t i = f * per; i < (f + 1) * per; ++i) on += m[i] > 0.0;
                        v += 100.0 * double(on) / double(per) / double(moments.size());
                    }
                } else if (row == 1) {
                    size_t on = 0;
                    for (size_t i = f * per; i < (f + 1) * per; ++i) {
                        bool any1 = false;
                        for (const auto& m : moments) any1 = any1 || m[i] > 0.0;
                        on += any1;
                    }
                    v = 100.0 * double(on) / double(per);
                } else {
                    const size_t lag = row == 2 ? 3 : size_t(2 * period); // 2 * period = same letter two repeats later
                    int c = 0;
                    for (size_t t = 0; t + lag < moments.size(); ++t, ++c) v += lab::cosine(moments[t], moments[t + lag], f * per, (f + 1) * per);
                    v /= std::max(1, c);
                }
                std::printf(" %7.3f", v);
            }
            std::printf("\n");
        }
    }
    // The same for a space-free comparison: how alike is each word to the lone space pattern?
    const Patterns sp = storeAll(s, {" "}, kStore);
    double simSpace = 0.0;
    for (const auto& p : P) simSpace += lab::cosine(p, sp[0]) / double(n);
    std::printf("  similarity of a word's shape to the shape of the space character alone: %.3f\n", simSpace);
    return 0;
}

// Word load: how many words can the matrix hold? Words are learned one after another (streamed);
// after 8, 16, 32 and 64 words every word learned so far is completed from its first 3 letters
// (heard once) and counted as right when what follows resembles its own stored shape more than
// any other learned word's. Reported for the learning matrix and an untrained twin (whose only
// help is the echo of the 3 letters). Chance = 1 / number of words.
int runWordLoadTest(const Config& cfg, uint64_t maxWords) {
    const std::vector<std::string> words = {
        "apple", "river", "storm", "candy", "light", "mouse", "bench", "think", "green", "house", "plant", "water", "smile",
        "dream", "cloud", "tiger", "fruit", "queen", "jolly", "knife", "lemon", "night", "ocean", "piano", "robot", "sugar",
        "table", "uncle", "voice", "whale", "zebra", "brick", "chair", "dance", "eagle", "flame", "ghost", "honey", "ivory",
        "jewel", "koala", "maple", "nurse", "olive", "pearl", "quilt", "raven", "snake", "torch", "urban", "vivid", "wheat",
        "yacht", "amber", "blaze", "crown", "drift", "elbow", "frost", "grape", "hinge", "index", "joker", "karma"};
    const size_t total = std::min<size_t>(maxWords ? maxWords : words.size(), words.size());
    std::printf("Word load: words learned one after another, each completed from its first 3 letters\n");
    std::printf("  words   learned: right  (first half / second half learned)  own-vs-best-other | untrained: right | chance\n");
    struct Row {
        size_t n, right, rightOld, rightNew;
        double margin;
    };
    std::vector<Row> rows[2];
    for (int learning = 1; learning >= 0; --learning) {
        Session s(cfg, learning == 1);
        Patterns stored;
        size_t next = 8;
        for (size_t w = 0; w < total; ++w) {
            const std::string item = words[w] + " ";
            stored.push_back(s.present(item, kStore, 1.0f, true, kStore / 2));
            s.silence(kGap, true);
            if (w + 1 == next || w + 1 == total) {
                Row r{w + 1, 0, 0, 0, 0.0};
                std::string mistakes;
                for (size_t k = 0; k <= w; ++k) {
                    s.present(words[k].substr(0, 3), 3, 1.0f, false, UINT64_MAX);
                    const std::vector<double> after = s.silence(T(40), false, 0);
                    s.silence(kGap, false);
                    double own = lab::cosine(after, stored[k]), best = -1.0;
                    size_t bestJ = k;
                    for (size_t j = 0; j <= w; ++j) {
                        if (j == k) continue;
                        const double sim = lab::cosine(after, stored[j]);
                        if (sim > best) {
                            best = sim;
                            bestJ = j;
                        }
                    }
                    const bool ok = own > best;
                    if (!ok && learning) mistakes += " " + words[k] + ">" + words[bestJ];
                    r.right += ok;
                    (k < (w + 1) / 2 ? r.rightOld : r.rightNew) += ok;
                    r.margin += (own - best) / double(w + 1);
                }
                rows[learning].push_back(r);
                std::printf("  .. %s, %zu words: %zu right (%.0f%%), first half %zu, second half %zu\n",
                            learning ? "learned" : "untrained", r.n, r.right, 100.0 * double(r.right) / double(r.n),
                            r.rightOld, r.rightNew);
                if (!mistakes.empty()) std::printf("     mixed up (word>taken for):%s\n", mistakes.c_str());
                std::fflush(stdout);
                next *= 2;
            }
        }
    }
    for (size_t i = 0; i < rows[1].size(); ++i) {
        const Row& l = rows[1][i];
        const Row& u = rows[0][i];
        std::printf("  %4zu    %3zu (%3.0f%%)   %3zu / %3zu                          %+.3f           | %3zu (%3.0f%%)      | %.0f%%\n",
                    l.n, l.right, 100.0 * double(l.right) / double(l.n), l.rightOld, l.rightNew, l.margin, u.right,
                    100.0 * double(u.right) / double(u.n), 100.0 / double(l.n));
    }
    return 0;
}

// Pair load: memory that an echo cannot fake. Word pairs ("apple river ") are learned one pair
// after another; then only the first word is heard once and the silence that follows is compared
// with the shape every pair's SECOND word had while it was being learned. Right = the own
// partner is the best match. The cue contains nothing of the partner, so an untrained twin is
// at chance (1 / number of pairs). Tested after 4, 8, 16 and 32 pairs.
int runPairLoadTest(const Config& cfg, uint64_t maxPairs) {
    const std::vector<std::string> words = {
        "apple", "river", "storm", "candy", "light", "mouse", "bench", "think", "green", "house", "plant", "water",
        "smile", "dream", "cloud", "tiger", "fruit", "queen", "jolly", "knife", "lemon", "night", "ocean", "piano",
        "robot", "sugar", "table", "uncle", "voice", "whale", "zebra", "brick", "chair", "dance", "eagle", "flame",
        "ghost", "honey", "ivory", "jewel", "koala", "maple", "nurse", "olive", "pearl", "quilt", "raven", "snake",
        "torch", "urban", "vivid", "wheat", "yacht", "amber", "blaze", "crown", "drift", "elbow", "frost", "grape",
        "hinge", "index", "joker", "karma", "badge", "cabin", "daisy", "fence", "glove", "horse", "lunar", "medal",
        "novel", "orbit", "paint", "radio", "salad", "tower", "unity", "vapor", "waltz", "youth", "acorn", "beach",
        "comet", "dodge", "ember", "fable", "giant", "hedge", "igloo", "jelly", "kneel", "ladle", "mango", "noble",
        "oasis", "pilot", "quest", "rider", "shelf", "tulip", "usher", "valve", "wagon", "bacon", "cider", "denim",
        "easel", "fudge", "gecko", "hound", "irony", "juice", "kayak", "llama", "mocha", "nacho", "otter", "panda",
        "quota", "rhino", "sauce", "thumb", "ultra", "viola", "wrist", "yeast", "zesty", "blend", "crisp", "dwarf",
        "flint", "grain", "haste", "latch", "mirth", "notch", "plume", "quirk", "roost", "slate", "trout", "whisk",
        "bloom", "chalk", "dough", "flock", "gravy", "hymns", "knack", "lilac", "mossy", "nudge", "oxide", "prism",
        "quack", "reef", "scarf", "twine", "vault", "woven", "alarm", "berry", "coral", "diner", "eject", "fairy",
        "gland", "hatch", "inlet", "jumbo", "kiosk", "lodge", "mural", "nylon", "onion", "perch", "rally", "sheep",
        "spine", "tango", "udder", "venom", "witty", "brave", "clerk", "dusty", "ferry", "glory", "honor", "inbox",
        "jazzy", "kitty", "mercy", "noisy", "opera", "pouch", "rusty", "sunny"};
    const size_t total = std::min<size_t>(maxPairs ? maxPairs : words.size() / 2, words.size() / 2);
    std::printf("Pair load: pairs learned one after another; hear the first word, is the second one recalled?\n");
    double finalRecall[2] = {0.0, 0.0}; // [untrained, learned] true-recall share at the last stage
    // Big runs (NCM_PAIR_FAST set): learning matrix only, no link readouts, tested from 16 pairs on.
    const bool fast = std::getenv("NCM_PAIR_FAST") != nullptr;
    // Reference shapes free of the cue: every partner word heard alone by a fresh untrained matrix
    // with the same wiring. An echo of the cue word has nothing in common with them.
    Patterns alone;
    if (!fast) {
        Session ref(cfg, false);
        for (size_t p = 0; p < total; ++p) {
            alone.push_back(ref.present(words[2 * p + 1] + " ", kStore, 1.0f, false, kStore / 2));
            ref.silence(kGap, false);
        }
    }
    const uint64_t rest = std::getenv("NCM_REST") ? std::stoull(std::getenv("NCM_REST")) : 0;
    const bool watch = std::getenv("NCM_REPLAY_WATCH") != nullptr;
    for (int learning = 1; learning >= (fast ? 1 : 0); --learning) {
        Session s(cfg, learning == 1);
        std::vector<unsigned> replayed(total, 0);
        Patterns partner; // shape of each pair's second word while the pair was learned
        Patterns cueInPair; // shape of each pair's first word while the pair was learned
        size_t next = fast ? 16 : 4;
        for (size_t p = 0; p < total; ++p) {
            const std::string a = words[2 * p] + " ", bw = words[2 * p + 1] + " ";
            const std::string pair = a + bw;
            // The pair is said, then a pause, then said again (10 times). Without the pause the
            // stream "apple river apple river" also teaches "river then apple", and an order rule
            // that strengthens "earlier -> later" and weakens "later -> earlier" cancels itself.
            std::vector<double> shape, cueShape;
            const int repeats = 10;
            for (int r = 0; r < repeats; ++r) {
                for (size_t pos = 0; pos < pair.size(); ++pos) {
                    s.present(std::string(1, pair[pos]), 1, 1.0f, true, UINT64_MAX);
                    if (r >= repeats / 2) s.accumulate(pos >= a.size() ? shape : cueShape);
                }
                s.silence(T(30), true);
            }
            partner.push_back(shape);
            cueInPair.push_back(cueShape);
            s.silence(kGap, true);
            // Optional rest after each pair (NCM_REST = 1D ticks of silence with learning on), the
            // same for every version; quiet-time replay can only happen in such pauses.
            if (rest > 0) {
                if (!watch) {
                    s.silence(rest, true);
                } else {
                    // Replay watch: what plays during the rest? Each snapshot (every 8 ticks) is
                    // compared with every stored pair's shape (cue + partner while learned).
                    std::vector<double> snap;
                    uint64_t active = 0, snaps = 0;
                    double best = 0.0, mean = 0.0;
                    for (uint64_t t = 0; t < rest; ++t) {
                        s.silence(1, true);
                        if (t % 8 != 7) continue;
                        snap.clear();
                        s.accumulate(snap);
                        ++snaps;
                        double tot = 0.0;
                        for (double x : snap) tot += x;
                        if (tot <= 0.0) continue;
                        ++active;
                        double b1 = -1.0, m1 = 0.0;
                        size_t who = 0;
                        for (size_t k = 0; k <= p; ++k) {
                            std::vector<double> both = partner[k];
                            for (size_t i = 0; i < both.size() && i < cueInPair[k].size(); ++i) both[i] += cueInPair[k][i];
                            const double c = lab::cosine(snap, both);
                            m1 += c / double(p + 1);
                            if (c > b1) { b1 = c; who = k; }
                        }
                        best += b1;
                        mean += m1;
                        replayed[who] += 1;
                    }
                    if (p + 1 == next || p + 1 == total) {
                        std::printf("  replay watch after pair %zu: active %llu of %llu snapshots, best match %.3f vs "
                                    "average %.3f; replays per pair:", p + 1, (unsigned long long)active,
                                    (unsigned long long)snaps, active ? best / double(active) : 0.0,
                                    active ? mean / double(active) : 0.0);
                        for (size_t k = 0; k <= p; ++k) std::printf(" %u", replayed[k]);
                        std::printf("\n");
                    }
                }
            }
            if (p + 1 == next || p + 1 == total) {
                size_t right = 0, rightOld = 0, rightNew = 0, rightAlone = 0, rightOwn = 0;
                std::vector<bool> recalledOwn(p + 1, false);
                double margin = 0.0, marginOwn = 0.0;
                std::string mistakes, mistakesOwn;
                // Yardstick in the matrix's own present language: every partner word heard alone
                // by this same matrix now (no learning), a few repeats each.
                // Heard in listening mode (as while learning: learned links muted by novelty) but
                // with learning paused, so stored memories do not fire into the reference shapes.
                Patterns ownAlone;
                s.setLearning(false);
                for (size_t k = 0; k <= p; ++k) {
                    const std::string bw2 = words[2 * k + 1] + " ";
                    ownAlone.push_back(s.present(bw2, 4 * bw2.size(), 1.0f, true, bw2.size()));
                    s.silence(kGap, false);
                }
                s.setLearning(learning == 1);
                // Stored links, read directly (no dynamics): the learned flow from each cue word's
                // own shape into every partner's shape, and back. Tells "not stored" from "stored
                // but not expressed".
                if (learning && !fast) {
                    Patterns cueAlone;
                    s.setLearning(false);
                    for (size_t k = 0; k <= p; ++k) {
                        const std::string aw = words[2 * k] + " ";
                        cueAlone.push_back(s.present(aw, 4 * aw.size(), 1.0f, true, aw.size()));
                        s.silence(kGap, false);
                    }
                    s.setLearning(true);
                    size_t linkRight = 0;
                    double fOwn = 0.0, fOther = 0.0, fBack = 0.0, fSelf = 0.0;
                    for (size_t k = 0; k <= p; ++k) {
                        const double own = s.flow(cueAlone[k], ownAlone[k]);
                        double best = 0.0;
                        for (size_t j = 0; j <= p; ++j)
                            if (j != k) best = std::max(best, s.flow(cueAlone[k], ownAlone[j]));
                        linkRight += own > best;
                        fOwn += own / double(p + 1);
                        fOther += best / double(p + 1);
                        fBack += s.flow(ownAlone[k], cueAlone[k]) / double(p + 1);
                        fSelf += s.flow(cueAlone[k], cueAlone[k]) / double(p + 1);
                    }
                    // The same with what all words share removed (mean shape subtracted): the part of
                    // each shape that is specific to the word.
                    {
                        const size_t dim = cueAlone[0].size();
                        std::vector<double> mean(dim, 0.0);
                        for (size_t k = 0; k <= p; ++k)
                            for (size_t i = 0; i < dim; ++i) mean[i] += (cueAlone[k][i] + ownAlone[k][i]) / double(2 * (p + 1));
                        Patterns cc = cueAlone, pc = ownAlone;
                        for (size_t k = 0; k <= p; ++k)
                            for (size_t i = 0; i < dim; ++i) {
                                cc[k][i] -= mean[i];
                                pc[k][i] -= mean[i];
                            }
                        size_t right2 = 0, rightBack = 0;
                        double o2 = 0.0, x2 = 0.0, b2 = 0.0;
                        for (size_t k = 0; k <= p; ++k) {
                            const double own = s.flow(cc[k], pc[k]);
                            double best = -1e9, bestB = -1e9;
                            for (size_t j = 0; j <= p; ++j)
                                if (j != k) {
                                    best = std::max(best, s.flow(cc[k], pc[j]));
                                    bestB = std::max(bestB, s.flow(pc[k], cc[j]));
                                }
                            const double back = s.flow(pc[k], cc[k]);
                            right2 += own > best;
                            rightBack += back > bestB;
                            o2 += own / double(p + 1);
                            x2 += best / double(p + 1);
                            b2 += back / double(p + 1);
                        }
                        std::printf("     WORD-SPECIFIC LINKS (shared part removed): cue->own partner strongest for %zu of %zu "
                                    "(own %+.4f, best other %+.4f); partner->cue %zu of %zu (%+.4f)\n",
                                    right2, p + 1, o2, x2, rightBack, p + 1, b2);
                    }
                    std::printf("     STORED LINKS cue->partner: strongest link points to the right partner for %zu of %zu\n"
                                "       strength: cue->own partner %.4f, cue->best other partner %.4f, partner->cue %.4f, cue->itself %.4f\n",
                                linkRight, p + 1, fOwn, fOther, fBack, fSelf);
                }
                for (size_t k = 0; k <= p; ++k) {
                    const std::string cue = words[2 * k] + " ";
                    s.present(cue, cue.size(), 1.0f, false, UINT64_MAX);
                    const std::vector<double> after = s.silence(T(40), false, 0);
                    s.silence(kGap, false);
                    const double own = lab::cosine(after, partner[k]);
                    double best = -1.0;
                    size_t bestJ = k;
                    for (size_t j = 0; j <= p; ++j) {
                        if (j == k) continue;
                        const double sim = lab::cosine(after, partner[j]);
                        if (sim > best) {
                            best = sim;
                            bestJ = j;
                        }
                    }
                    const bool ok = own > best;
                    right += ok;
                    (k < (p + 1) / 2 ? rightOld : rightNew) += ok;
                    margin += (own - best) / double(p + 1);
                    if (!ok && learning) mistakes += " " + words[2 * k] + ">" + words[2 * bestJ + 1];
                    // Strict count: against the partner words' shapes heard alone (fresh matrix).
                    if (!fast) {
                        double ownA = lab::cosine(after, alone[k]), bestA = -1.0;
                        for (size_t j = 0; j <= p; ++j)
                            if (j != k) bestA = std::max(bestA, lab::cosine(after, alone[j]));
                        rightAlone += ownA > bestA;
                    }
                    double ownO = lab::cosine(after, ownAlone[k]), bestO = -1.0;
                    size_t bestOJ = k;
                    for (size_t j = 0; j <= p; ++j) {
                        if (j == k) continue;
                        const double sim = lab::cosine(after, ownAlone[j]);
                        if (sim > bestO) {
                            bestO = sim;
                            bestOJ = j;
                        }
                    }
                    rightOwn += ownO > bestO;
                    recalledOwn[k] = ownO > bestO;
                    marginOwn += (ownO - bestO) / double(p + 1);
                    if (ownO <= bestO && learning) mistakesOwn += " " + words[2 * k] + ">" + words[2 * bestOJ + 1];
                }
                std::printf("  .. %s, %zu pairs: %zu right (%.0f%%), older half %zu, newer half %zu, margin %+.3f, chance %.0f%%\n",
                            learning ? "learned" : "untrained", p + 1, right, 100.0 * double(right) / double(p + 1), rightOld,
                            rightNew, margin, 100.0 / double(p + 1));
                std::printf("     strict (partner shape heard alone): %zu right (%.0f%%)\n", rightAlone,
                            100.0 * double(rightAlone) / double(p + 1));
                std::printf("     TRUE RECALL (partner heard alone by this same matrix): %zu of %zu right (%.0f%%), margin %+.3f\n",
                            rightOwn, p + 1, 100.0 * double(rightOwn) / double(p + 1), marginOwn);
                finalRecall[learning] = double(rightOwn) / double(p + 1);
                if (!mistakesOwn.empty()) std::printf("       wrong (cue>recalled):%s\n", mistakesOwn.c_str());
                // Per pair (learning matrix): the stored link measured on the words as they were
                // heard together (cue -> own partner vs the strongest cue -> other partner), and
                // whether the partner was truly recalled. Tells "not stored" from "not expressed".
                if (learning && !fast) {
                    std::printf("     per pair: link to own partner / strongest other (stored?) | recalled?\n");
                    for (size_t k = 0; k <= p; ++k) {
                        const double own = s.flow(cueInPair[k], partner[k]);
                        double best = 0.0;
                        for (size_t j = 0; j <= p; ++j)
                            if (j != k) best = std::max(best, s.flow(cueInPair[k], partner[j]));
                        const bool recalled = recalledOwn[k];
                        std::printf("       %2zu %-6s %.3f / %.3f %-4s | %s\n", k + 1, words[2 * k].c_str(), own, best,
                                    own > best ? "yes" : "NO", recalled ? "yes" : "NO");
                    }
                }
                // Calibration: does a partner word heard inside its pair look like itself heard alone?
                if (!fast) {
                    size_t match = 0;
                    double ownSim = 0.0, otherSim = 0.0;
                    for (size_t k = 0; k <= p; ++k) {
                        const double own = lab::cosine(partner[k], alone[k]);
                        double best = -1.0;
                        for (size_t j = 0; j <= p; ++j)
                            if (j != k) best = std::max(best, lab::cosine(partner[k], alone[j]));
                        match += own > best;
                        ownSim += own / double(p + 1);
                        otherSim += best / double(p + 1);
                    }
                    std::printf("     calibration: word inside its pair vs the same word alone: %zu of %zu recognisable (alike %.3f, best other %.3f)\n",
                                match, p + 1, ownSim, otherSim);
                }
                if (!mistakes.empty()) std::printf("     wrong partner (cue>recalled):%s\n", mistakes.c_str());
                std::fflush(stdout);
                next *= 2;
            }
        }
    }
    // Pass: the learning matrix truly recalls at least 75% of the partners at the last stage, and
    // the untrained twin (which can only guess or echo) at most 25%.
    const bool pass = finalRecall[1] >= 0.75 && finalRecall[0] <= 0.25;
    std::printf("  word pairs: true recall %.0f%% (need >= 75%%) vs untrained %.0f%% (need <= 25%%): %s\n",
                100.0 * finalRecall[1], 100.0 * finalRecall[0], pass ? "PASS" : "FAIL");
    return pass ? 0 : 3;
}

// Pair links (diagnostic): is order stored? One pair ("apple river ", said 10 times with pauses)
// is learned. The 3D state at every tick position of the pair is summed over the last repeats;
// the learned flow between those states is printed as a table (row = from, column = to).
// A stored order shows as strong flow from a position to the positions that follow it and weak
// flow back. Also printed: how alike the states are, and the flow into an unrelated word.
int runPairLinksTest(const Config& cfg) {
    const std::string pair = "apple river ";
    Session s(cfg, true);
    std::vector<std::vector<double>> P(pair.size());
    const int repeats = 10;
    for (int r = 0; r < repeats; ++r) {
        for (size_t pos = 0; pos < pair.size(); ++pos) {
            s.present(std::string(1, pair[pos]), 1, 1.0f, true, UINT64_MAX);
            if (r >= repeats / 2) s.accumulate(P[pos]);
        }
        s.silence(T(30), true);
    }
    s.setLearning(false); // listening mode, links unchanged: the stored pair does not fire into it
    const std::vector<double> other = s.present("candy ", 24, 1.0f, true, 6);
    s.setLearning(true);
    s.silence(kGap, false);
    std::printf("Pair links after learning %c%s%c (10 times, with pauses): learned flow from row to column\n        ", '"', pair.c_str(), '"');
    for (size_t j = 0; j < pair.size(); ++j) std::printf("   %c   ", pair[j] == ' ' ? '_' : pair[j]);
    std::printf(" | candy\n");
    for (size_t i = 0; i < pair.size(); ++i) {
        std::printf("   %c   ", pair[i] == ' ' ? '_' : pair[i]);
        for (size_t j = 0; j < pair.size(); ++j) std::printf(" %.3f ", s.flow(P[i], P[j]));
        std::printf(" | %.3f\n", s.flow(P[i], other));
    }
    std::printf("  how alike the states are (row vs column):\n");
    for (size_t i = 0; i < pair.size(); ++i) {
        std::printf("   %c   ", pair[i] == ' ' ? '_' : pair[i]);
        for (size_t j = 0; j < pair.size(); ++j) std::printf(" %.3f ", lab::cosine(P[i], P[j]));
        std::printf(" | %.3f\n", lab::cosine(P[i], other));
    }
    // Word level: first word (positions 0-5) -> second word (6-11) and back.
    std::vector<double> A, B;
    for (size_t i = 0; i < pair.size(); ++i) {
        auto& t = i < 6 ? A : B;
        if (t.size() != P[i].size()) t.assign(P[i].size(), 0.0);
        for (size_t k = 0; k < t.size(); ++k) t[k] += P[i][k];
    }
    std::printf("  word level: apple->river %.4f, river->apple %.4f, apple->apple %.4f, river->river %.4f, apple->candy %.4f\n",
                s.flow(A, B), s.flow(B, A), s.flow(A, A), s.flow(B, B), s.flow(A, other));
    return 0;
}

// Context (diagnostic): does a word keep its identity whatever came before it? Untrained matrix,
// listening only. "river" is heard after a pause, after "apple", after "storm" and after itself;
// its shape (3D state summed over its own letters) is compared across those settings, per field,
// and with a different word ("candy") heard in the same settings.
int runWordContextTest(const Config& cfg) {
    const char* names[kFields] = {"Input", "Memory", "Reasoning", "Output"};
    Session s(cfg, false);
    auto shapeOf = [&](const std::string& before, const std::string& word) {
        std::vector<double> shape;
        for (int r = 0; r < 6; ++r) {
            if (!before.empty()) s.present(before, before.size(), 1.0f, true, UINT64_MAX);
            // The matrix shows a chunk only after hearing it (one chunk = 3 ticks), so the word's
            // shape is read 3 ticks late: from its 4th tick to 3 ticks after its end.
            for (size_t i = 0; i < word.size(); ++i) {
                s.present(std::string(1, word[i]), 1, 1.0f, true, UINT64_MAX);
                if (r >= 2 && i >= 3) s.accumulate(shape);
            }
            for (int t = 0; t < 3; ++t) {
                s.silence(1, false);
                if (r >= 2) s.accumulate(shape);
            }
            s.silence(T(30) - 3, false);
        }
        s.silence(kGap, false);
        return shape;
    };
    const std::vector<double> rPause = shapeOf("", "river "), rApple = shapeOf("apple ", "river "),
                              rStorm = shapeOf("storm ", "river "), rSelf = shapeOf("river ", "river "),
                              rPause2 = shapeOf("", "river "), cPause = shapeOf("", "candy "),
                              cApple = shapeOf("apple ", "candy ");
    const size_t per = rPause.size() / kFields;
    std::printf("Word context (untrained): how alike is \"river\" to itself in different settings, per field\n");
    std::printf("                                          Input  Memory Reason Output\n");
    auto row = [&](const char* label, const std::vector<double>& a, const std::vector<double>& b2) {
        std::printf("  %-38s", label);
        for (uint32_t f = 0; f < kFields; ++f) std::printf("  %.3f", lab::cosine(a, b2, f * per, (f + 1) * per));
        std::printf("\n");
    };
    row("river after a pause, heard twice", rPause, rPause2);
    row("river after a pause vs after apple", rPause, rApple);
    row("river after apple vs after storm", rApple, rStorm);
    row("river after a pause vs after river", rPause, rSelf);
    row("river vs candy, both after a pause", rPause, cPause);
    row("river vs candy, both after apple", rApple, cApple);
    (void)names;
    return 0;
}

// Recall detail (diagnostic): what exactly comes back. 8 memories are stored as in CP3, then each
// is cued (40%). The cells active at the end of the cue (last 10 ticks) are compared with each
// stored memory's cells (binary sets, per field and overall):
//   complete = share of the own memory's cells that came back,
//   clean    = share of the active cells that belong to the own memory,
//   other    = share of the active cells that belong to the best-matching other memory,
//   size     = active cells / own memory's cells (1 = same size as stored).
// Shows whether a failure is an incomplete memory, the right memory plus extra cells, or the
// wrong memory. Learning matrix and untrained twin.
int runRecallDetailTest(const Config& cfg) {
    const std::vector<std::string> items = {"a", "k", "z", "m", "q", "e", "t", "w"};
    std::printf("Recall detail: %zu memories, each cued at %.0f%%; cells active at the end of the cue vs stored cells\n",
                items.size(), 100.0 * kCueFraction);
    for (int learning = 1; learning >= 0; --learning) {
        Session s(cfg, learning == 1);
        const Patterns stored = storeAll(s, items, kStore);
        // A stored memory's cells: voxel channels active in at least a quarter of its storage ticks.
        const double storedTicks = double(kStore - kStore / 2);
        std::vector<std::vector<uint8_t>> sets;
        for (const auto& p : stored) {
            std::vector<uint8_t> set(p.size());
            for (size_t i = 0; i < p.size(); ++i) set[i] = p[i] > 0.0 && p[i] / storedTicks > 0.02;
            sets.push_back(set);
        }
        const size_t per = stored[0].size() / kFields;
        std::printf("  %s\n    memory  complete  clean   other(best)  size   | recalled as | full cue: complete clean | 40%% cue reaches (complete 40/full)\n", learning ? "LEARNED" : "UNTRAINED");
        double sumC = 0, sumP = 0, sumO = 0, sumS = 0, sumRatio = 0;
        for (size_t k = 0; k < items.size(); ++k) {
            std::vector<double> window;
            for (uint64_t t = 0; t < kCue; ++t) {
                s.present(items[k], 1, kCueFraction, false, UINT64_MAX);
                if (t + 10 >= kCue) s.accumulate(window);
            }
            s.silence(kGap, false);
            std::vector<uint8_t> act(window.size());
            size_t nAct = 0;
            for (size_t i = 0; i < window.size(); ++i) nAct += (act[i] = window[i] / 10.0 > 0.02);
            auto overlap = [&](const std::vector<uint8_t>& set, size_t& setSize) {
                size_t both = 0;
                setSize = 0;
                for (size_t i = 0; i < set.size(); ++i) {
                    setSize += set[i];
                    both += set[i] && act[i];
                }
                return both;
            };
            size_t ownSize = 0;
            const size_t ownBoth = overlap(sets[k], ownSize);
            size_t bestOther = 0, bestJ = k;
            for (size_t j = 0; j < sets.size(); ++j) {
                if (j == k) continue;
                size_t sz = 0;
                const size_t o = overlap(sets[j], sz);
                if (o > bestOther) {
                    bestOther = o;
                    bestJ = j;
                }
            }
            double best = -1.0;
            size_t endsIn = k;
            for (size_t j = 0; j < stored.size(); ++j) {
                const double sim = lab::cosine(window, stored[j]);
                if (sim > best) {
                    best = sim;
                    endsIn = j;
                }
            }
            // Reference: the full letter, same window length, same counting.
            std::vector<double> full;
            for (uint64_t t = 0; t < kCue; ++t) {
                s.present(items[k], 1, 1.0f, false, UINT64_MAX);
                if (t + 10 >= kCue) s.accumulate(full);
            }
            s.silence(kGap, false);
            size_t fullBoth = 0, fullAct = 0;
            for (size_t i = 0; i < full.size(); ++i) {
                const bool a = full[i] / 10.0 > 0.02;
                fullAct += a;
                fullBoth += a && sets[k][i];
            }
            const double complete = ownSize ? double(ownBoth) / double(ownSize) : 0.0;
            const double completeFull = ownSize ? double(fullBoth) / double(ownSize) : 0.0;
            const double cleanFull = fullAct ? double(fullBoth) / double(fullAct) : 0.0;
            const double clean = nAct ? double(ownBoth) / double(nAct) : 0.0;
            const double other = nAct ? double(bestOther) / double(nAct) : 0.0;
            const double size = ownSize ? double(nAct) / double(ownSize) : 0.0;
            sumC += complete / items.size();
            sumP += clean / items.size();
            sumO += other / items.size();
            sumS += size / items.size();
            std::printf("    %-6s  %.2f      %.2f    %.2f (%s)     %.2f   | %-4s%-9s | %.2f     %.2f  | %.0f%%\n", items[k].c_str(), complete, clean,
                        other, items[bestJ].c_str(), size, items[endsIn].c_str(), endsIn == k ? "" : " <-WRONG", completeFull,
                        cleanFull, completeFull > 0 ? 100.0 * complete / completeFull : 0.0);
            sumRatio += completeFull > 0 ? complete / completeFull / items.size() : 0.0;
        }
        std::printf("    mean    %.2f      %.2f    %.2f          %.2f   | 40%% cue reaches %.0f%% of what the full letter brings\n", sumC, sumP, sumO, sumS, 100.0 * sumRatio);
        (void)per;
    }
    return 0;
}

} // namespace ncm
