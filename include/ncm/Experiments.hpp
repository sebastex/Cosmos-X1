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
int runRecallTest(const Config& cfg, const RecallOptions& opt);

// Stage 1 memory suite (MemorySuite.cpp). Each returns 0 on pass.
int runCapacityTest(const Config& cfg);   // CP3: the right memory among 8
int runEfficiencyTest(const Config& cfg); // CP4: usable after short exposure
int runStreamedTest(const Config& cfg);   // CP5: memories from streamed text
int runContinualTest(const Config& cfg);  // CP6: new learning does not erase old memories
int runOrderTest(const Config& cfg);      // CP7: early order signal
int runMemorySuite(const Config& cfg);    // all of the above plus the recall test

} // namespace ncm
