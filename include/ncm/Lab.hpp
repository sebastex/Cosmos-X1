#pragma once
// Shared machinery for memory experiments (Stage 1 tests): driving a matrix with text,
// surprise-gated learning, and comparing activity patterns.

#include <algorithm>
#include <climits>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <string>
#include <vector>

#include "ncm/CharacterCodebook.hpp"
#include "ncm/Matrix.hpp"
#include "ncm/Random.hpp"
#include "ncm/Scheduler.hpp"

namespace ncm::lab {

inline double cosine(const std::vector<double>& a, const std::vector<double>& b, size_t begin = 0,
                     size_t end = SIZE_MAX) {
    end = std::min({end, a.size(), b.size()});
    double dot = 0.0, na = 0.0, nb = 0.0;
    for (size_t i = begin; i < end; ++i) {
        dot += a[i] * b[i];
        na += a[i] * a[i];
        nb += b[i] * b[i];
    }
    return (na > 0.0 && nb > 0.0) ? dot / std::sqrt(na * nb) : 0.0;
}

// Keeps a fixed share of a fingerprint's lines: a partial cue.
inline std::vector<uint32_t> thin(const std::vector<uint32_t>& fp, float fraction, uint64_t seed, char c) {
    std::vector<uint32_t> kept;
    for (uint32_t s : fp)
        if (hashUniform(seed, 0xC0E + uint64_t(uint8_t(c)), s) < fraction) kept.push_back(s);
    return kept;
}

// Drives one matrix: text in, level clocks, surprise-gated learning, encoding/recall mode.
class Session {
public:
    Session(const Config& cfg, bool learning, bool silenceSuppressed = true)
        : cfg_(cfg), learning_(learning), silenceSuppressed_(silenceSuppressed),
          m_(std::make_unique<NeuralCellularMatrix>(cfg)),
          codebook_(cfg.surfaceLines(), cfg.target_activity, cfg.itemSeed()) {}

    // One 1D tick. `fingerprint` is what enters the sensory surface (nullptr = silence);
    // `actual` is the full fingerprint of the character being heard, for the surprise check.
    void tick(const std::vector<uint32_t>* fingerprint, const std::vector<uint32_t>* actual, bool allowLearning) {
        // Onset after a pause: the slower clocks start a fresh cycle, so a word is cut into the
        // same chunks whether it follows a pause or another word.
        if (fingerprint && wasSilent_ && cfg_.clock_reset > 0.5f) clock_.resetPhase();
        if (fingerprint && wasSilent_ && cfg_.learning.word_context > 0.0f) m_->resetWordContext();
        wasSilent_ = fingerprint == nullptr;
        if (fingerprint) {
            // Surprise (spec Section 5A): read the matrix's guess before the character
            // arrives. Only external text produces surprise.
            const double guess = actual ? m_->motorOverlap(*actual) : 0.0;
            surpriseSum_ += 1.0 - guess;
            ++surpriseCount_;
            m_->setSensoryInput(*fingerprint);
            // While storing, surprise sets encoding mode. During recall the cue is familiar
            // material being retrieved, so the matrix runs in recall mode (M = 0). Until the
            // motor path learns to predict (Stage 3) surprise cannot tell the two apart itself.
            setMode(allowLearning ? (cfg_.encoding_full > 0.5f ? 1.0f : float(1.0 - guess)) : 0.0f, false);
        } else {
            m_->clearSensoryInput();
            setMode(silenceSuppressed_ ? 1.0f : 0.0f, true);
        }
        // Quiet-time replay: after a long enough silence with learning on, learned links
        // transmit again (recall mode) and spontaneous kicks start stored memories playing.
        quietTicks_ = fingerprint ? 0 : quietTicks_ + 1;
        const bool replay = cfg_.learning.replay > 0.0f && learning_ && allowLearning &&
                            double(quietTicks_) > double(cfg_.learning.replay_after);
        if (replay) {
            mode_ = 0.0f;
            m_->setModulator(0.0f);
        }
        m_->setReplay(replay ? cfg_.learning.replay : 0.0f, cfg_.learning.replay_share);

        m_->step1D();
        const LevelScheduler::Tick t = clock_.advance();
        if (t.sheet) m_->step2D();
        if (t.voxel) {
            m_->step3D();
            float modulator = surpriseCount_ ? float(surpriseSum_ / double(surpriseCount_)) : 0.0f;
            surpriseSum_ = 0.0;
            surpriseCount_ = 0;
            // Neuromodulator kinetics: the learning signal builds up over modulator_tau 3D ticks
            // after surprise begins (and decays in silence), so the wave of activity that passes
            // through the fields as an input arrives is not stored; the settled pattern is.
            if (cfg_.learning.modulator_tau > 0.0f) {
                gate_ += (modulator - gate_) / std::max(1.0f, cfg_.learning.modulator_tau);
                modulator = gate_;
            }
            if (replay) {
                m_->learn(cfg_.learning.replay_rate);
            } else if (learning_ && allowLearning) {
                modSum_ += modulator;
                ++modCount_;
                m_->learn(modulator);
                lastModulator_ = modulator;
            }
        }
    }

    // Presents `text` (cycling through its characters, one per tick) for `ticks` ticks, with
    // each fingerprint thinned to `fraction` (1 = full input). Returns the summed 3D state
    // from tick `recordFrom` on (empty if recordFrom >= ticks).
    std::vector<double> present(const std::string& text, uint64_t ticks, float fraction, bool allowLearning,
                                uint64_t recordFrom) {
        std::vector<double> acc;
        for (uint64_t t = 0; t < ticks; ++t) {
            const char c = text[t % text.size()];
            if (cfg_.space_silent > 0.5f && c == ' ') { // a word gap is a pause
                tick(nullptr, nullptr, allowLearning);
                if (t >= recordFrom) accumulate(acc);
                continue;
            }
            const auto& full = codebook_.fingerprint(char32_t(uint8_t(c)));
            if (fraction >= 1.0f) {
                tick(&full, &full, allowLearning);
            } else {
                const auto cue = thin(full, fraction, cfg_.itemSeed(), c);
                tick(&cue, &full, allowLearning);
            }
            if (cfg_.clock_reset > 0.5f && c == ' ') clock_.resetPhase();
            if (cfg_.learning.word_context > 0.0f && c == ' ') m_->resetWordContext();
            if (t >= recordFrom) accumulate(acc);
        }
        return acc;
    }

    // Silence for `ticks` ticks; returns the summed 3D state from tick `recordFrom` on.
    std::vector<double> silence(uint64_t ticks, bool allowLearning, uint64_t recordFrom = UINT64_MAX) {
        std::vector<double> acc;
        for (uint64_t t = 0; t < ticks; ++t) {
            tick(nullptr, nullptr, allowLearning);
            if (t >= recordFrom) accumulate(acc);
        }
        return acc;
    }

    void accumulate(std::vector<double>& acc) const {
        const auto& s = m_->voxelState();
        if (acc.size() != s.size()) acc.assign(s.size(), 0.0);
        for (size_t i = 0; i < s.size(); ++i) acc[i] += s[i];
    }

    void clearActivity() { m_->clearActivity(); }
    void setLearning(bool on) { learning_ = on; } // diagnostic: pause learning, keep the mode
    double flow(const std::vector<double>& from, const std::vector<double>& to) { return m_->plasticFlow(from, to); }
    const NeuralCellularMatrix& matrix() const { return *m_; }
    const CharacterCodebook& codebook() const { return codebook_; }
    float lastModulator() const { return lastModulator_; }
    float lastMode() const { return mode_; } // encoding/recall mode applied (1 = learned links suppressed)
    ~Session() {
        if (std::getenv("NCM_PROFILE") && m_)
            std::fprintf(stderr, "profile: lines %.1fs, sheets %.1fs, voxels %.1fs, learning %.1fs\n", m_->time1D, m_->time2D,
                         m_->time3D, m_->timeLearn);
    }
    // Mean learning signal over the learning steps since the last reset (diagnostic).
    double meanModulator() const { return modCount_ ? modSum_ / double(modCount_) : 0.0; }
    void resetModulatorMean() { modSum_ = 0.0; modCount_ = 0; }

private:
    Config cfg_;
    bool learning_;
    bool silenceSuppressed_;
    std::unique_ptr<NeuralCellularMatrix> m_;
    CharacterCodebook codebook_;
    LevelScheduler clock_;
    double surpriseSum_ = 0.0;
    uint64_t surpriseCount_ = 0;
    float lastModulator_ = 0.0f;
    float gate_ = 0.0f; // learning signal with neuromodulator kinetics
    double modSum_ = 0.0;
    uint64_t modCount_ = 0;
    float mode_ = 1.0f; // encoding/recall mode actually applied (the matrix starts suppressed)
    bool wasSilent_ = true; // the previous tick had no input
    uint64_t quietTicks_ = 0; // 1D ticks of silence so far (quiet-time replay)

    // Encoding/recall mode with neuromodulator kinetics (mode_tau, 1D ticks): after input
    // ends, suppression builds up gradually, so memory circuits stay in recall mode briefly
    // and what comes next can play out. While input is present the mode follows it at once:
    // new material is encoded as it arrives (audit: gradual onset weakened new memories).
    // 0 = instantaneous.
    void setMode(float target, bool silence) {
        const float tau = cfg_.learning.mode_tau;
        if (silence && tau > 0.0f && target > mode_) mode_ += (target - mode_) / std::max(1.0f, tau);
        else mode_ = target;
        m_->setModulator(mode_);
    }
};

// How specifically a set of cues recalled their own stored patterns.
struct Specificity {
    bool identifies = true; // every cue is most similar to its own pattern
    double own = 0.0;       // mean similarity to own pattern
    double margin = 0.0;    // mean (own - best other)
};

// similarity[k][j] = cue k vs stored pattern j. `cues` selects which rows count
// (all rows when empty); every column competes.
inline Specificity specificity(const std::vector<std::vector<double>>& similarity,
                               const std::vector<size_t>& cues = {}) {
    Specificity s;
    std::vector<size_t> rows = cues;
    if (rows.empty())
        for (size_t k = 0; k < similarity.size(); ++k) rows.push_back(k);
    for (size_t k : rows) {
        double best = -1.0;
        for (size_t j = 0; j < similarity[k].size(); ++j)
            if (j != k) best = std::max(best, similarity[k][j]);
        s.identifies = s.identifies && similarity[k][k] > best;
        s.own += similarity[k][k];
        s.margin += similarity[k][k] - best;
    }
    if (!rows.empty()) {
        s.own /= double(rows.size());
        s.margin /= double(rows.size());
    }
    return s;
}

inline std::vector<std::vector<double>> similarityMatrix(const std::vector<std::vector<double>>& cues,
                                                         const std::vector<std::vector<double>>& stored) {
    std::vector<std::vector<double>> sim(cues.size(), std::vector<double>(stored.size(), 0.0));
    for (size_t k = 0; k < cues.size(); ++k)
        for (size_t j = 0; j < stored.size(); ++j) sim[k][j] = cosine(cues[k], stored[j]);
    return sim;
}

} // namespace ncm::lab
