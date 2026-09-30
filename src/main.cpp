// Cosmos X1, Stage 0: the Neural Cellular Matrix skeleton.
// Streams text into the sensory surface, runs the 1D -> 2D -> 3D -> 4D ladder on
// the CPU, and records how activity spreads and settles.

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <memory>
#include <new>
#include <string>
#include <vector>

#include "ncm/CharacterCodebook.hpp"
#include "ncm/Experiments.hpp"
#include "ncm/Matrix.hpp"
#include "ncm/Scheduler.hpp"
#include "ncm/Viewer.hpp"

namespace {

struct Options {
    std::string preset = "dev";
    std::string rule = "evolved"; // "evolved" (default) or "starting" (the hand-set rule)
    uint64_t ticks = 600;
    uint64_t inputTicks = 200;
    uint64_t snapEvery = 50;
    std::string outDir = "out";
    std::string text = "the cat sat on the mat. ";
    uint64_t seed = 0;
    bool seedSet = false;
    bool quiet = false;
    std::string test = "stage0";
    bool clearGaps = false;
    bool silenceRecall = false; // diagnostic: full-strength recall mode during silence
    bool earlyExit = false;     // suite: stop after recall if learning clearly hurts it
    uint64_t storeTicks = 0;    // recall test: exposure per stored item (0 = default)
    float cueFraction = 0.0f;
    double timeScale = 1.0;     // tests: multiplies storing, gap and cue durations   // recall test: share of the fingerprint kept in cues (0 = default)
    std::vector<std::string> patterns; // empty = the recall test's default
    std::vector<std::string> settings;
};

void usage() {
    std::cout << "usage: cosmos_x1 [--test stage0|recall] [--preset tiny|dev|full] [--ticks N] [--input-ticks N]\n"
                 "                 [--snap N] [--out DIR] [--text \"...\"] [--seed N] [--quiet]\n"
                 "                 [--set name=value]...\n"
                 "settings: "
              << ncm::settingNames() << "\n";
}

bool parse(int argc, char** argv, Options& o) {
    for (int i = 1; i < argc; ++i) {
        const std::string a = argv[i];
        auto value = [&]() -> std::string {
            if (i + 1 >= argc) throw std::invalid_argument("missing value for " + a);
            return argv[++i];
        };
        if (a == "--preset") o.preset = value();
        else if (a == "--rule") o.rule = value();
        else if (a == "--ticks") o.ticks = std::stoull(value());
        else if (a == "--input-ticks") o.inputTicks = std::stoull(value());
        else if (a == "--snap") o.snapEvery = std::max<uint64_t>(1, std::stoull(value()));
        else if (a == "--out") o.outDir = value();
        else if (a == "--text") o.text = value();
        else if (a == "--seed") { o.seed = std::stoull(value()); o.seedSet = true; }
        else if (a == "--set") o.settings.push_back(value());
        else if (a == "--quiet") o.quiet = true;
        else if (a == "--test") o.test = value();
        else if (a == "--clear-gaps") o.clearGaps = true;
        else if (a == "--silence-recall") o.silenceRecall = true;
        else if (a == "--early-exit") o.earlyExit = true;
        else if (a == "--store-ticks") o.storeTicks = std::stoull(value());
        else if (a == "--cue-fraction") o.cueFraction = std::stof(value());
        else if (a == "--time-scale") o.timeScale = std::stod(value());
        else if (a == "--patterns") {
            o.patterns.clear();
            std::string list = value(), item;
            for (char c : list) {
                if (c == ',') { o.patterns.push_back(item); item.clear(); }
                else item += c;
            }
            o.patterns.push_back(item);
        }
        else if (a == "--help" || a == "-h") { usage(); return false; }
        else throw std::invalid_argument("unknown argument " + a);
    }
    if (o.text.empty()) o.text = " ";
    return true;
}

const char* kFieldNames[ncm::kFields] = {"Input", "Memory", "Reasoning", "Output"};

// Pearson correlation of two equally sized states; identical constant states count as 1.
double correlation(const ncm::AVec<float>& a, const ncm::AVec<float>& b) {
    if (a.size() != b.size() || a.empty()) return 0.0;
    double ma = 0.0, mb = 0.0;
    for (size_t i = 0; i < a.size(); ++i) {
        ma += a[i];
        mb += b[i];
    }
    ma /= double(a.size());
    mb /= double(b.size());
    double cov = 0.0, va = 0.0, vb = 0.0;
    for (size_t i = 0; i < a.size(); ++i) {
        const double da = a[i] - ma, db = b[i] - mb;
        cov += da * db;
        va += da * da;
        vb += db * db;
    }
    if (va == 0.0 || vb == 0.0) return (va == vb && ma == mb) ? 1.0 : 0.0;
    return cov / std::sqrt(va * vb);
}

void printLevel(const char* name, const ncm::LevelStats& ls) {
    std::printf("  %-3s", name);
    for (uint32_t f = 0; f < ncm::kFields; ++f)
        std::printf("  %s %6.2f%% (mean %.4f)", kFieldNames[f], 100.0 * ls.active_fraction[f], ls.mean[f]);
    std::printf("\n");
}

} // namespace

int main(int argc, char** argv) {
    Options opt;
    try {
        if (!parse(argc, argv, opt)) return 0;
    } catch (const std::exception& e) {
        std::cerr << "error: " << e.what() << "\n";
        usage();
        return 2;
    }

    ncm::Config cfg;
    try {
        cfg = ncm::makePreset(opt.preset);
        if (opt.rule == "evolved") ncm::applyEvolvedRule(cfg);
        else if (opt.rule != "starting") throw std::invalid_argument("unknown rule '" + opt.rule + "' (use evolved or starting)");
        for (const auto& s : opt.settings) ncm::applySetting(cfg, s);
    } catch (const std::exception& e) {
        std::cerr << "error: " << e.what() << "\n";
        return 2;
    }
    if (opt.seedSet) cfg.seed = opt.seed;
    ncm::setTestTimeScale(opt.timeScale);

    if (opt.test == "recall") {
        std::printf("COSMOS X1: NEURAL CELLULAR MATRIX, preset %s\n", opt.preset.c_str());
        ncm::RecallOptions ro;
        ro.quiet = opt.quiet;
        ro.clearBetween = opt.clearGaps;
        ro.silenceSuppressed = !opt.silenceRecall;
        if (!opt.patterns.empty()) ro.patterns = opt.patterns;
        if (opt.storeTicks > 0) ro.storeTicks = opt.storeTicks;
        if (opt.cueFraction > 0.0f) ro.cueFraction = opt.cueFraction;
        ro.storeTicks = uint64_t(double(ro.storeTicks) * opt.timeScale + 0.5);
        ro.gapTicks = uint64_t(double(ro.gapTicks) * opt.timeScale + 0.5);
        ro.cueTicks = uint64_t(double(ro.cueTicks) * opt.timeScale + 0.5);
        return ncm::runRecallTest(cfg, ro);
    }
    if (opt.test == "reliability") {
        ncm::RecallOptions ro;
        if (!opt.patterns.empty()) ro.patterns = opt.patterns;
        return ncm::runReliabilityTest(cfg, ro);
    }
    if (opt.test == "capacity") return ncm::runCapacityTest(cfg);
    if (opt.test == "efficiency") return ncm::runEfficiencyTest(cfg);
    if (opt.test == "streamed") return ncm::runStreamedTest(cfg);
    if (opt.test == "continual") return ncm::runContinualTest(cfg);
    if (opt.test == "order") return ncm::runOrderTest(cfg);
    if (opt.test == "context") return ncm::runContextTest(cfg);
    if (opt.test == "profile") return ncm::runProfileTest(cfg);
    if (opt.test == "retention") return ncm::runRetentionTest(cfg);
    if (opt.test == "streamdiag") return ncm::runStreamDiagTest(cfg);
    if (opt.test == "health") return ncm::runHealthTest(cfg);
    if (opt.test == "hum") return ncm::runHumTest(cfg);
    if (opt.test == "chain") return ncm::runChainTest(cfg);
    if (opt.test == "overlap") return ncm::runOverlapTest(cfg);
    if (opt.test == "pairload") return ncm::runPairLoadTest(cfg, opt.storeTicks);
    if (opt.test == "wordload") return ncm::runWordLoadTest(cfg, opt.storeTicks);
    if (opt.test == "wordshape") return ncm::runWordShapeTest(cfg, true, opt.storeTicks);
    if (opt.test == "wordshapenospace") return ncm::runWordShapeTest(cfg, false);
    if (opt.test == "wordcapacity") return ncm::runWordCapacityTest(cfg);
    if (opt.test == "wordcontinual") return ncm::runWordContinualTest(cfg);
    if (opt.test == "wordcontinualswap") return ncm::runWordContinualTest(cfg, true);
    if (opt.test == "discriminate") return ncm::runDiscriminationTest(cfg);
    if (opt.test == "interfere") return ncm::runInterferenceTest(cfg);
    if (opt.test == "completion") return ncm::runCompletionTest(cfg, opt.storeTicks > 0 ? opt.storeTicks : 3);
    if (opt.test == "settle") return ncm::runSettleTest(cfg, opt.patterns.empty() ? "a" : opt.patterns[0]);
    if (opt.test == "occupancy") return ncm::runOccupancyTest(cfg, opt.patterns.empty() ? "a" : opt.patterns[0]);
    if (opt.test == "drift") return ncm::runDriftTest(cfg, opt.patterns.empty() ? "a" : opt.patterns[0], opt.storeTicks);
    if (opt.test == "suite") return ncm::runMemorySuite(cfg, opt.earlyExit);
    if (opt.test != "stage0") {
        std::cerr << "error: unknown test '" << opt.test
                  << "' (use stage0, recall, capacity, efficiency, streamed, continual, order or suite)\n";
        return 2;
    }

    std::printf("========================================================\n");
    std::printf(" COSMOS X1: NEURAL CELLULAR MATRIX, Stage 0 (CPU)\n");
    std::printf("========================================================\n");
    std::printf(" preset %s: fields %u^3 x 4, sheets %ux%u, lines %u\n", opt.preset.c_str(), cfg.field_dim,
                cfg.sheet_dim, cfg.sheet_dim, cfg.line_len);
    std::printf(" cells: 3D %zu, 2D %zu, 1D %zu (total %zu)\n", cfg.voxels(), cfg.sheetCells(), cfg.lineCells(),
                cfg.voxels() + cfg.sheetCells() + cfg.lineCells());

    std::unique_ptr<ncm::NeuralCellularMatrix> matrix;
    try {
        matrix = std::make_unique<ncm::NeuralCellularMatrix>(cfg);
    } catch (const std::bad_alloc&) {
        std::cerr << "error: not enough memory for preset '" << opt.preset << "'\n";
        return 1;
    }
    ncm::NeuralCellularMatrix& m = *matrix;
    ncm::CharacterCodebook codebook(cfg.surfaceLines(), cfg.target_activity, cfg.itemSeed());

    std::printf(" memory: %.1f MB\n", double(m.memoryBytes()) / (1024.0 * 1024.0));
    std::printf(" sensory surface: %zu lines, %zu per character fingerprint\n", cfg.surfaceLines(),
                codebook.linesPerCharacter());
    std::printf(" streaming \"%s\" for %llu ticks, then silence until tick %llu\n\n", opt.text.c_str(),
                (unsigned long long)opt.inputTicks, (unsigned long long)opt.ticks);

    std::filesystem::create_directories(opt.outDir);

    ncm::LevelScheduler clock;
    using Clock = std::chrono::steady_clock;
    std::array<double, 3> seconds{};
    const double eps = 1e-4;
    std::array<double, ncm::kFields> peakVoxelMean{};
    std::array<uint64_t, ncm::kFields> firstLit{}; // first snapshot tick a field's voxels became active (0 = never)
    double peakLevelMean = 0.0;                     // highest mean at any level/field/snapshot
    double peakInputActive = 0.0;                   // highest share of strongly-active Input cells (2D/3D)
    double peakTotalMean3 = 0.0;
    std::vector<std::pair<uint64_t, double>> history; // (tick, total 3D mean) per snapshot
    ncm::MatrixStats last{};
    // Voxel state 100 ticks before the end, compared with the final state to detect freezing.
    const uint64_t lateTick = opt.ticks > 100 ? opt.ticks - 100 : 0;
    ncm::AVec<float> lateState;

    for (uint64_t t = 0; t < opt.ticks; ++t) {
        if (t < opt.inputTicks)
            m.setSensoryInput(codebook.fingerprint(char32_t(uint8_t(opt.text[t % opt.text.size()]))));
        else
            m.clearSensoryInput();

        const auto t0 = Clock::now();
        m.step1D();
        const auto t1 = Clock::now();
        seconds[0] += std::chrono::duration<double>(t1 - t0).count();

        const ncm::LevelScheduler::Tick tick = clock.advance();
        if (tick.sheet) {
            const auto a = Clock::now();
            m.step2D();
            seconds[1] += std::chrono::duration<double>(Clock::now() - a).count();
        }
        if (tick.voxel) {
            const auto a = Clock::now();
            m.step3D();
            seconds[2] += std::chrono::duration<double>(Clock::now() - a).count();
        }

        if (t + 1 == lateTick) lateState = m.voxelState();

        if ((t + 1) % opt.snapEvery == 0 || t + 1 == opt.ticks) {
            last = m.computeStats();
            double totalMean3 = 0.0;
            for (uint32_t f = 0; f < ncm::kFields; ++f) {
                peakVoxelMean[f] = std::max(peakVoxelMean[f], last.voxel.mean[f]);
                if (!firstLit[f] && last.voxel.mean[f] > eps) firstLit[f] = t + 1;
                peakLevelMean = std::max({peakLevelMean, last.voxel.mean[f], last.sheet.mean[f], last.line.mean[f]});
                totalMean3 += last.voxel.mean[f];
            }
            peakTotalMean3 = std::max(peakTotalMean3, totalMean3);
            history.emplace_back(t + 1, totalMean3);
            peakInputActive = std::max({peakInputActive, last.voxel.active_fraction[0], last.sheet.active_fraction[0]});
            if (!opt.quiet) {
                std::printf("tick %llu%s\n", (unsigned long long)(t + 1), t < opt.inputTicks ? " (input on)" : "");
                printLevel("3D", last.voxel);
                printLevel("2D", last.sheet);
                printLevel("1D", last.line);
            }

            char name[64];
            std::snprintf(name, sizeof(name), "tick_%06llu.bmp", (unsigned long long)(t + 1));
            ncm::writeSnapshot(m, (std::filesystem::path(opt.outDir) / name).string());
        }
    }

    const double ms1 = 1000.0 * seconds[0] / double(std::max<uint64_t>(1, clock.ticks1D()));
    const double ms2 = 1000.0 * seconds[1] / double(std::max<uint64_t>(1, clock.ticks2D()));
    const double ms3 = 1000.0 * seconds[2] / double(std::max<uint64_t>(1, clock.ticks3D()));
    const double total = seconds[0] + seconds[1] + seconds[2];
    std::printf("\nTiming (tick rate, needed to set the tag lifetime, spec Section 5B)\n");
    std::printf("  1D step %.2f ms x %llu, 2D step %.2f ms x %llu, 3D step %.2f ms x %llu\n", ms1,
                (unsigned long long)clock.ticks1D(), ms2, (unsigned long long)clock.ticks2D(), ms3,
                (unsigned long long)clock.ticks3D());
    std::printf("  %.1f 1D ticks per second overall (%.1f s total)\n", double(clock.ticks1D()) / total, total);

    // Stage 0 is done when a pattern injected into the Input field spreads through
    // the ladder, stays sparse, and settles once input stops (spec Section 10).
    // Before learning, the matrix must be input-driven with a fading memory: once
    // input stops, activity falls back toward rest. A matrix that keeps generating
    // its own activity responds to its history instead of its input and cannot
    // represent anything (found by the Stage 1 recall test). Holding chosen
    // patterns is the job of learned connections in Stage 1.
    const bool reached = firstLit[0] != 0;
    bool spread = reached, inputFirst = reached;
    for (uint32_t f = 1; f < ncm::kFields; ++f) {
        spread = spread && firstLit[f] != 0;
        inputFirst = inputFirst && (firstLit[f] == 0 || firstLit[f] >= firstLit[0]);
    }
    const double sparseLimit = 5.0 * cfg.target_activity;
    const bool sparse = peakLevelMean <= sparseLimit && peakInputActive > 0.0;
    const double frozenCorrelation = correlation(lateState, m.voxelState());

    auto activityAt = [&](uint64_t tick) {
        double value = 0.0;
        for (const auto& [t, a] : history)
            if (t <= tick) value = a;
        return value;
    };
    const double atInputEnd = activityAt(opt.inputTicks);
    const double finalTotalMean3 = history.empty() ? 0.0 : history.back().second;
    const bool longEnough = opt.ticks >= opt.inputTicks + 200;
    const bool fades = atInputEnd > 0.0 && finalTotalMean3 <= 0.5 * atInputEnd;
    // Any activity that remains must still be changing, not saturated and frozen.
    const bool notFrozen = finalTotalMean3 < eps || frozenCorrelation < 0.98;
    const bool settles = longEnough && fades && notFrozen && sparse;

    std::printf("\nStage 0 check\n");
    std::printf("  reached Input field:              %s\n", reached ? "yes" : "NO");
    std::printf("  spread to Memory/Reasoning/Output: %s (peak 3D activity relative to Input:", spread ? "yes" : "NO");
    for (uint32_t f = 1; f < ncm::kFields; ++f)
        std::printf(" %s %.0f%%", kFieldNames[f], peakVoxelMean[0] > 0.0 ? 100.0 * peakVoxelMean[f] / peakVoxelMean[0] : 0.0);
    std::printf(")\n");
    std::printf("  Input lit first:                  %s (first active at ticks", inputFirst ? "yes" : "NO");
    for (uint32_t f = 0; f < ncm::kFields; ++f) std::printf(" %llu", (unsigned long long)firstLit[f]);
    std::printf(")\n");
    std::printf("  sparse (mean <= %.2f, some cells strongly on): %s (peak mean %.4f, peak Input active %.2f%%)\n",
                sparseLimit, sparse ? "yes" : "NO", peakLevelMean, 100.0 * peakInputActive);
    std::printf("  settles (fades toward rest, not frozen): %s (3D activity at input end %.4f, end %.4f; "
                "pattern correlation over the last 100 ticks %.3f)%s\n",
                settles ? "yes" : "NO", atInputEnd, finalTotalMean3, frozenCorrelation,
                longEnough ? "" : " [run at least 200 ticks past input]");
    std::printf("  field gains at end (gain control):");
    for (uint32_t f = 0; f < ncm::kFields; ++f) std::printf(" %s %.2f", kFieldNames[f], m.fieldGains()[f]);
    std::printf("\n");
    std::printf("  snapshots: %s\n", std::filesystem::absolute(opt.outDir).string().c_str());
    return (reached && spread && inputFirst && sparse && settles) ? 0 : 3;
}
