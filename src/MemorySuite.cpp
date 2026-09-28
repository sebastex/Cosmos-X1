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
        std::printf("  diagnostic, %s: cue vs stored (apple river stone)\n", l ? "learned" : "untrained");
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
                s.present(first, 40, 1.0f, true, UINT64_MAX);
                s.present(second, 40, 1.0f, true, UINT64_MAX);
                s.silence(40, true);
            }
        // Clean reference pattern for every item: encoding mode (as the other checks store
        // their references) with learning paused. In recall mode the learned forward link
        // leaked the second item into the first item's reference (audit 2026-09-27).
        auto reference = [&](const std::string& item) {
            s.setLearning(false);
            auto p = s.present(item, 40, 1.0f, true, 20);
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
            const auto cueFirst = s.present(first, 30, kCueFraction, false, 15);
            const auto afterFirst = s.silence(30, false, 0);
            s.silence(kGap - 30, false);
            const auto cueSecond = s.present(second, 30, kCueFraction, false, 15);
            const auto afterSecond = s.silence(30, false, 0);
            s.silence(kGap - 30, false);
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

} // namespace ncm
