#pragma once
#include <chrono>
#include <cstdlib>
#include <string>
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
#include <random>
#include <array>
#include <map>

#include "ncm/CharacterCodebook.hpp"
#include "ncm/Matrix.hpp"
#include "ncm/Monitor.hpp"
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
        : cfg_(clockScaled(cfg)), learning_(learning), silenceSuppressed_(silenceSuppressed),
          m_(std::make_unique<NeuralCellularMatrix>(cfg_)),
          codebook_(cfg.surfaceLines(), cfg.target_activity, cfg.itemSeed()), monitor_(Monitor::fromEnv(cfg)) {
        clock_.setFast(cfg.clock_fast > 0.5f);
    }

    // One 1D tick. `fingerprint` is what enters the sensory surface (nullptr = silence);
    // `actual` is the full fingerprint of the character being heard, for the surprise check.
    void tick(const std::vector<uint32_t>* fingerprint, const std::vector<uint32_t>* actual, bool allowLearning) {
        // Onset after a pause: the slower clocks start a fresh cycle, so a word is cut into the
        // same chunks whether it follows a pause or another word.
        if (fingerprint && wasSilent_ && cfg_.clock_reset > 0.5f) clock_.resetPhase();
        if (fingerprint && wasSilent_) m_->wordBoundary();
        const bool wasSilentBefore = wasSilent_;
        wasSilent_ = fingerprint == nullptr;
        if (fingerprint) {
            // Surprise (spec Section 5A): read the matrix's guess before the character
            // arrives. Only external text produces surprise.
            // Learn by predicting: the guess is how much the cells' learned links predicted (last 3D step).
            const double guess = cfg_.predict_surprise > 0.5f ? 1.0 - double(m_->lastSurprise())
                                                                       : actual ? m_->motorOverlap(*actual) : 0.0;
            surpriseSum_ += 1.0 - guess;
            ++surpriseCount_;
            if (cfg_.self_sense > 0.0f && (cfg_.self_sense_end < 0.5f || heard_ == ' ')) { // the brain feels its own state
                std::vector<uint32_t> merged = *fingerprint;
                const auto& inner = selfFingerprint();
                merged.insert(merged.end(), inner.begin(), inner.end());
                m_->setSensoryInput(merged);
            } else {
                m_->setSensoryInput(*fingerprint);
            }
            // Imitation. speak_learn 1: the mouth shows the letter being heard. speak_learn 2: the letter about
            // to come (set by the lesson with mouthTeacher), so it learns to say what comes next.
            if (cfg_.inner_sense > 0.0f && innerFeed_) { // it feels its own state
                if (cfg_.inner_lines > 0.5f) m_->setInnerLines(selfFingerprint());
                else m_->clampInnerSheets(selfFingerprint());
            } else {
                m_->releaseInnerSheets();
                m_->clearInnerLines();
            }
            if (cfg_.speak_learn > 1.5f && allowLearning && mouthNext_) m_->clampMotorSheets(*mouthNext_);
            else if (cfg_.speak_learn > 0.5f && cfg_.speak_learn <= 1.5f && allowLearning) m_->clampMotorSheets(actual ? *actual : *fingerprint);
            else m_->releaseMotorSheets();
            // While storing, surprise sets encoding mode. During recall the cue is familiar
            // material being retrieved, so the matrix runs in recall mode (M = 0). Until the
            // motor path learns to predict (Stage 3) surprise cannot tell the two apart itself.
            // familiar_rate_only: familiarity sets only how strongly a word is learnt; a lesson stays in
            // encoding mode (learned loops muted), since recall mode in lessons made one blob.
            setMode(allowLearning ? (cfg_.familiar_rate_only > 0.5f ? 1.0f : float(1.0 - guess)) : 0.0f, false);
        } else {
            m_->clearSensoryInput();
            m_->releaseMotorSheets();
            m_->releaseInnerSheets();
            m_->clearInnerLines();
            setMode(silenceSuppressed_ ? 1.0f : 0.0f, true);
        }

        auto t0 = Clock::now();
        m_->step1D();
        lap(0, t0);
        // clock_reset 2: a word ends at its space or where silence begins; there every level takes
        // one look at once, so the top level sees the whole word (cues and lessons alike).
        const bool wordEnd = fingerprint && heard_ == ' ';
        if (cfg_.clock_reset > 1.5f && (wordEnd || (!fingerprint && !wasSilentBefore))) clock_.lookNow();
        const LevelScheduler::Tick t = clock_.advance();
        if (t.sheet) {
            m_->step2D();
            lap(1, t0);
            if (learning_ && allowLearning) m_->learn2D(lastModulator_);
            lap(2, t0);
        }
        if (monitor_) {
            monitorEvent_ = Monitor::Event{};
            monitorEvent_.sheet = t.sheet;
            monitorEvent_.voxel = t.voxel;
            monitorEvent_.fourD = t.fourD;
            monitorEvent_.sensory = fingerprint != nullptr;
            monitorEvent_.learning = allowLearning; // teaching moment (the same in an untrained twin)
            monitorEvent_.heard = fingerprint ? heard_ : 0;
        }
        if (t.voxel) {
            m_->setTick4D(t.fourD);
            m_->step3D();
            m_->updateAch(fingerprint != nullptr);
            lap(t.fourD ? 4 : 3, t0);
            if (watchActivity_) { // read only: what the brain does while hearing and in silence
                const int b = fingerprint ? 0 : 1;
                const bool all = actSteps_++ % 8 == 0; // the sheet and line levels on every 8th step
                const MatrixStats st = m_->computeStats(!all);
                for (uint32_t f = 0; f < kFields; ++f) {
                    act_.voxel[b][f] += st.voxel.active_fraction[f];
                    act_.voxelMean[b][f] += st.voxel.mean[f];
                    if (all) act_.sheet[b][f] += st.sheet.active_fraction[f], act_.line[b][f] += st.line.active_fraction[f];
                }
                ++act_.steps[b];
                if (all) ++act_.allSteps[b];
            }
            const bool selfWatch = cfg_.self_sense > 0.0f || cfg_.valence > 0.0f || cfg_.inner_sense > 0.0f;
            if (cfg_.predict_surprise > 0.5f || cfg_.predict_watch > 0.5f || selfWatch) m_->predictSurprise();
            if (selfWatch && fingerprint) { // how well it predicted now vs. its running average: good / bad
                const float hit = 1.0f - m_->lastSurprise();
                mood_ = hit - hitAvg_;
                hitAvg_ += (hit - hitAvg_) / 50.0f;
                const float d = m_->lastSurprise() - surAvg_; // its usual surprise and spread
                surAvg_ += d / 50.0f;
                surVar_ += (d * d - surVar_) / 50.0f;
                surFelt_ += (m_->lastSurprise() - surFelt_) / std::max(1.0f, cfg_.inner_tau); // inner_tau: smoothed
            }
            float modulator = surpriseCount_ ? float(surpriseSum_ / double(surpriseCount_)) : 0.0f;
            // familiar_floor: a known word still learns at least this much (gentler familiarity).
            if (cfg_.familiar_floor > 0.0f) modulator = cfg_.familiar_floor + (1.0f - cfg_.familiar_floor) * modulator;
            surpriseSum_ = 0.0;
            surpriseCount_ = 0;
            if (cfg_.valence > 0.0f && fingerprint) modulator *= std::clamp(1.0f + cfg_.valence * mood_, 0.0f, 2.0f); // a need
            if (fingerprint) modulator = std::min(1.0f, modulator * arousal_); // charge: an emotionally charged moment learns harder
            if (learning_ && allowLearning) {
                modSum_ += modulator;
                ++modCount_;
                m_->learn(modulator);
                if (wordEnd) m_->learnWord(modulator); // settling: the whole word, learnt as one moment
                lastModulator_ = modulator;
            }
            lap(5, t0);
        }
        if (monitor_) {
            monitorEvent_.tick = clock_.ticks1D();
            // While only listening, the record shows its surprise (nothing is being learned from it).
            monitorEvent_.modulator = !allowLearning && cfg_.compare > 0.5f ? m_->somaticMismatch()
                                     : !allowLearning && (cfg_.predict_surprise > 0.5f || cfg_.predict_watch > 0.5f) ? m_->lastSurprise()
                                                                                                                      : lastModulator_;
            monitor_->record(*m_, monitorEvent_);
        }
        heard_ = 0;
    }

    // Presents `text` (cycling through its characters, one per tick) for `ticks` ticks, with
    // each fingerprint thinned to `fraction` (1 = full input). Returns the summed 3D state
    // from tick `recordFrom` on (empty if recordFrom >= ticks).
    // self_sense: the inner fingerprint of the brain's present state (surprise level x mood).
    const std::vector<uint32_t>& selfFingerprint() {
        const float sur = cfg_.inner_tau > 1.0f ? surFelt_ : m_->lastSurprise();
        int sBin = std::clamp(int(sur * 4.0f), 0, 3);
        if (cfg_.inner_relative > 0.5f) { // against its own usual surprise
            const float z = (sur - surAvg_) / std::sqrt(surVar_ + 1e-6f);
            sBin = z < -1.0f ? 0 : z < 0.0f ? 1 : z < 1.0f ? 2 : 3;
        }
        const int mBin = mood_ > 0.05f ? 2 : (mood_ < -0.05f ? 0 : 1);
        if (cfg_.inner_states > 1.5f && cfg_.inner_states < 2.5f) return stateFingerprint(sBin < 2 ? 1 : 10); // surer / less sure
        return stateFingerprint(sBin * 3 + mBin);
    }
    // Inner state id = surprise level (0..3, low to high) x 3 + mood (0 bad, 1 even, 2 good).
    const std::vector<uint32_t>& stateFingerprint(int id) {
        if (selfFp_[id].empty()) {
            const size_t lines = cfg_.surfaceLines();
            const float k0 = cfg_.inner_sense > 0.0f ? cfg_.inner_sense : cfg_.self_sense;
            const size_t k = std::max<size_t>(1, size_t(k0 * float(codebook_.linesPerCharacter())));
            std::mt19937_64 rng(cfg_.itemSeed() ^ (0x5E1FULL * uint64_t(id + 1)));
            std::vector<uint32_t> pick;
            while (pick.size() < k) {
                const uint32_t q = uint32_t(rng() % lines);
                if (std::find(pick.begin(), pick.end(), q) == pick.end()) pick.push_back(q);
            }
            std::sort(pick.begin(), pick.end());
            selfFp_[id] = pick;
        }
        return selfFp_[id];
    }
    // speak_learn 2: the letter the mouth should say next (0 = none).
    void innerFeed(bool on) { innerFeed_ = on; } // feel test: switch the inner sense off to read the brain's guess
    void mouthTeacher(char c) { mouthNext_ = c ? &codebook_.fingerprint(char32_t(uint8_t(c))) : nullptr; }
    // speak_plan: the mouth is shown a word's plan (all its letters, fainter with their place).
    void mouthPlan(const std::string& word, float fall) {
        plan_.clear();
        const size_t n = word.size();
        for (size_t i = 0; i < n; ++i) {
            const auto& fp = codebook_.fingerprint(char32_t(uint8_t(word[i])));
            const float share = 1.0f - fall * float(i) / float(std::max<size_t>(1, n - 1));
            const size_t keep = std::max<size_t>(1, size_t(std::ceil(share * float(fp.size()))));
            plan_.insert(plan_.end(), fp.begin(), fp.begin() + std::min(keep, fp.size()));
        }
        std::sort(plan_.begin(), plan_.end());
        plan_.erase(std::unique(plan_.begin(), plan_.end()), plan_.end());
        mouthNext_ = &plan_;
    }
    // word_shape: a word's own fixed random shape (made from the word on first use; as big as a letter's).
    const std::vector<uint32_t>& wordShape(const std::string& w) {
        auto it = wordFp_.find(w);
        if (it != wordFp_.end()) return it->second;
        uint64_t h = 1469598103934665603ULL; // FNV-1a of the word
        for (char c : w) h = (h ^ uint64_t(uint8_t(c))) * 1099511628211ULL;
        std::mt19937_64 rng(cfg_.itemSeed() ^ mix64(h ^ 0x30D5ULL));
        const size_t lines = cfg_.surfaceLines(), k = codebook_.linesPerCharacter();
        std::vector<uint8_t> taken(lines, 0);
        std::vector<uint32_t> pick;
        while (pick.size() < k) {
            const uint32_t q = uint32_t(rng() % lines);
            if (!taken[q]) taken[q] = 1, pick.push_back(q);
        }
        std::sort(pick.begin(), pick.end());
        return wordFp_[w] = pick;
    }
    const std::map<std::string, std::vector<uint32_t>>& wordShapes() const { return wordFp_; }
    void mouthShape(const std::vector<uint32_t>* fp) { mouthNext_ = fp; } // the mouth is shown a whole shape
    // word_shape: hear a whole word as its own shape, held for `hold` ticks (its space comes separately).
    void presentShape(const std::string& w, uint64_t hold, bool allowLearning) {
        const auto& fp = wordShape(w);
        for (uint64_t t = 0; t < hold; ++t) {
            heard_ = '#';
            tick(&fp, &fp, allowLearning);
        }
    }
    std::vector<double> present(const std::string& text, uint64_t ticks, float fraction, bool allowLearning,
                                uint64_t recordFrom) {
        std::vector<double> acc;
        for (uint64_t t = 0; t < ticks; ++t) {
            const char c = text[t % text.size()];
            const auto& full = codebook_.fingerprint(char32_t(uint8_t(c)));
            heard_ = c;
            if (fraction >= 1.0f) {
                tick(&full, &full, allowLearning);
            } else {
                const auto cue = thin(full, fraction, cfg_.itemSeed(), c);
                tick(&cue, &full, allowLearning);
            }
            if (cfg_.clock_reset > 0.5f && cfg_.clock_reset < 1.5f && c == ' ') clock_.resetPhase(); // (2: see tick)
            if (c == ' ') m_->wordBoundary();
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

    // Sleep for `ticks` ticks: no input, recall mode (learned links fully on), faint random nudges
    // (level, prob) so the brain falls into and replays what it knows, and on every 3D step the links
    // between voxels firing together are weakened by eta (unlearning; the slow stored part stays).
    void sleep(uint64_t ticks, float eta, float level, float prob) {
        m_->clearSensoryInput();
        m_->setNoise(level, prob);
        for (uint64_t t = 0; t < ticks; ++t) {
            setMode(0.0f, false);
            m_->step1D();
            const LevelScheduler::Tick tk = clock_.advance();
            if (tk.sheet) m_->step2D();
            if (tk.voxel) {
                m_->setTick4D(tk.fourD);
                m_->step3D();
                if (learning_) m_->learnSleep(eta);
            }
        }
        m_->setNoise(0.0f, 0.0f);
        wasSilent_ = true;
    }
    // Right or wrong for what was just done (+1 good, -1 bad), brain-wide; only while learning.
    void feedback(float signal) { if (learning_) m_->feedback(signal); }
    // charge (amygdala-like arousal, for Jung's complexes): learning strength x a while something is heard.
    void setArousal(float a) { arousal_ = std::max(0.0f, a); }
    // NCM_PROFILE=1: where the time goes (seconds per part of a tick), printed by profileReport().
    using Clock = std::chrono::steady_clock;
    void lap(int part, Clock::time_point& t0) {
        if (!profile_) return;
        const auto now = Clock::now();
        prof_[part] += std::chrono::duration<double>(now - t0).count();
        t0 = now;
    }
    std::string profileReport() const {
        if (!profile_) return "";
        const char* names[] = {"1D step", "2D step", "2D learn", "3D step", "3D step with 4D", "3D learn"};
        std::string s = "PROFILE";
        for (int k = 0; k < 6; ++k) s += std::string(" ") + names[k] + "=" + std::to_string(prof_[k]);
        return s + "\n";
    }
    // Activity per field, averaged over the 3D steps while hearing [0] and in silence [1] since the
    // last takeActivity() (watchActivity(true) turns the reading on; it changes nothing in the brain).
    struct ActivityLog {
        double voxel[2][kFields] = {}, voxelMean[2][kFields] = {}, sheet[2][kFields] = {}, line[2][kFields] = {};
        uint64_t steps[2] = {}, allSteps[2] = {};
    };
    void watchActivity(bool on) { watchActivity_ = on; }
    ActivityLog takeActivity() {
        ActivityLog a = act_;
        act_ = ActivityLog{};
        return a;
    }
    void setLearning(bool on) { learning_ = on; }
    bool learning() const { return learning_; } // diagnostic: pause learning, keep the mode
    double flow(const std::vector<double>& from, const std::vector<double>& to) { return m_->plasticFlow(from, to); }
    const NeuralCellularMatrix& matrix() const { return *m_; }
    // Save or restore this session's brain and clocks (one brain across runs).
    void state(StateFile& st) {
        m_->state(st);
        clock_.state(st);
        st.value(surpriseSum_);
        st.value(surpriseCount_);
        st.value(lastModulator_);
        st.value(modSum_);
        st.value(modCount_);
        st.value(mode_);
        st.value(wasSilent_);
    }
    const CharacterCodebook& codebook() const { return codebook_; }
    float lastModulator() const { return lastModulator_; }
    float lastMode() const { return mode_; } // encoding/recall mode applied (1 = learned links suppressed)
    ~Session() {
        if (std::getenv("NCM_PROFILE") && m_)
            std::fprintf(stderr, "profile: lines %.1fs, sheets %.1fs, voxels %.1fs, learning %.1fs; learning steps %llu, cells learning per step %.0f of %zu\n",
                         m_->time1D, m_->time2D, m_->time3D, m_->timeLearn, (unsigned long long)m_->learningStats().calls,
                         m_->learningStats().calls ? double(m_->learningStats().learners) / double(m_->learningStats().calls) : 0.0,
                         m_->voxelState().size() / C3);
    }
    // Mean learning signal over the learning steps since the last reset (diagnostic).
    double meanModulator() const { return modCount_ ? modSum_ / double(modCount_) : 0.0; }
    void resetModulatorMean() { modSum_ = 0.0; modCount_ = 0; }

private:
    Config cfg_;
    bool learning_;
    bool profile_ = std::getenv("NCM_PROFILE") && std::atoi(std::getenv("NCM_PROFILE")) > 0;
    double prof_[6] = {};
    bool watchActivity_ = false;
    ActivityLog act_;
    uint64_t actSteps_ = 0;
    bool silenceSuppressed_;
    std::unique_ptr<NeuralCellularMatrix> m_;
    CharacterCodebook codebook_;
    std::unique_ptr<Monitor> monitor_; // activity monitor (NCM_MONITOR), passive
    Monitor::Event monitorEvent_;
    char heard_ = 0; // the character being heard (for the monitor)
    bool innerFeed_ = true;
    float surAvg_ = 0.9f, surVar_ = 0.001f, surFelt_ = 0.9f;           // inner_relative: usual surprise and its spread
    float mood_ = 0.0f, hitAvg_ = 0.0f;              // valence / self_sense: good vs bad moments
    std::array<std::vector<uint32_t>, 12> selfFp_{};  // self_sense: inner-state fingerprints (made on first use)
    std::vector<uint32_t> plan_;                      // speak_plan: the word's plan on the mouth
    std::map<std::string, std::vector<uint32_t>> wordFp_; // word_shape: each word's own shape
    const std::vector<uint32_t>* mouthNext_ = nullptr; // speak_learn 2: the letter the mouth is taught to say next
    LevelScheduler clock_;
    double surpriseSum_ = 0.0;
    uint64_t surpriseCount_ = 0;
    float lastModulator_ = 0.0f;
    double modSum_ = 0.0;
    uint64_t modCount_ = 0;
    float mode_ = 1.0f; // encoding/recall mode actually applied (the matrix starts suppressed)
    bool wasSilent_ = true; // the previous tick had no input
    float arousal_ = 1.0f; // charge: learning multiplier while hearing (1 = calm)

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
