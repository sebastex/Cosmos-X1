#pragma once
#include <string>
#include <vector>

#include "ncm/Config.hpp"

namespace ncm {

struct RecallOptions {
    // Stage 1 tests pattern memory, so each pattern is steady input: one character held on
    // the sensory surface. Sequences (e.g. words streamed letter by letter) are Stage 2.
    std::vector<std::string> patterns = {"a", "k", "z"};
    uint64_t storeTicks = 120;  // 1D ticks each pattern is presented while learning
    uint64_t gapTicks = 60;     // silence between presentations
    uint64_t cueTicks = 60;     // 1D ticks each partial cue is presented
    float cueFraction = 0.4f;   // share of each fingerprint kept in the cue
    bool quiet = false;
    bool clearBetween = false;  // diagnostic: silence all activity before each presentation
    // Mode during silence: true = learned connections stay turned down until input arrives
    // (memory circuits switch to recall when familiar input comes, not when all goes quiet);
    // false = full-strength recall mode in silence, which lets the last memory keep itself going.
    bool silenceSuppressed = true;
};

// Stage 1 test (spec Section 10): does a partial cue recall the whole stored pattern?
// Runs the same protocol on a learning matrix and on an identical matrix that never
// learns, and compares how closely each partial cue's activity matches the full pattern.
// Returns 0 when learning measurably improves recall and every cue finds its own pattern.
// If `specificityGain` is given, it receives the learned-minus-untrained specificity gain.
int runRecallTest(const Config& cfg, const RecallOptions& opt, double* specificityGain = nullptr);
int runReliabilityTest(const Config& cfg, const RecallOptions& opt); // the recall test's precondition only

// Stage 1 memory suite (MemorySuite.cpp). Each returns 0 on pass.
int runCapacityTest(const Config& cfg);   // CP3: the right memory among 8
int runEfficiencyTest(const Config& cfg); // CP4: usable after short exposure
int runStreamedTest(const Config& cfg);   // CP5: memories from streamed text
int runContinualTest(const Config& cfg);  // CP6: new learning does not erase old memories
int runOrderTest(const Config& cfg);      // CP7: early order signal
int runDriftTest(const Config& cfg, const std::string& item, uint64_t learnAfter = 0); // diagnostic: recall over time
int runContextTest(const Config& cfg); // diagnostic: context coding of shared letters
int runProfileTest(const Config& cfg); // diagnostic: memory profile under harsher conditions
int runRetentionTest(const Config& cfg); // CP6b: retention of 8 memories while 16 more are learned
int runStreamDiagTest(const Config& cfg); // diagnostic: learning signal and recall over time for streamed words
int runSettleTest(const Config& cfg, const std::string& item); // diagnostic: settling and fading time
int runCompletionTest(const Config& cfg, uint64_t prefix = 3); // CP5b: complete a streamed word from its start
int runHealthTest(const Config& cfg); // diagnostic: which subsystem misbehaves when memories blur
int runOccupancyTest(const Config& cfg, const std::string& item); // diagnostic: active voxels per depth layer
// Multiplies every test duration (storing, gaps, cues) by `scale` (default 1).
void setTestTimeScale(double scale);
// All of the above plus the recall test. With earlyExit, stops after the recall test when
// learning makes recall clearly worse than an untrained matrix (saves compute in searches).
int runMemorySuite(const Config& cfg, bool earlyExit = false);

} // namespace ncm
