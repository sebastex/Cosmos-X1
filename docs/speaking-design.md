# Speaking (design, 2026-11-18)

Goal: close the action loop. Cosmos says letters and hears itself, so it can learn
"I did this, and this happened". Parts only, no outcomes: it gets a mouth and an ear
on its own voice; what to say must be learnt.

## What already exists
- Ear: the sensory face (x = 0 of the Input field), `setSensoryInput`.
- Mouth: the motor face (x = N-1 of the Output field), `clampMotor` / `releaseMotor` /
  `readMotorExit`, and `CharacterCodebook::decode` (exit lines -> nearest letter, or silence).
- Unused so far: nothing drives the mouth in lessons, nothing reads it in tests.

## Parts to add (one at a time, each tested)
1. **Baseline measurement (no new mechanism).** In the quick test, after each cue, read the
   mouth every tick and decode it. Expect silence or noise. This is the yardstick.
2. **Imitation (`speak_learn`).** While a lesson letter is heard, the mouth is clamped with the
   same letter (a baby repeating sounds, spec 7B). The usual learning then ties
   "state just before letter k" to "mouth says letter k", i.e. the mouth learns to predict.
   Same links, same rules; only the mouth face gets a teacher signal.
3. **Speaking (`speak`).** After a cue the mouth is free. Each letter slot: decode the mouth;
   if it says a letter, that letter is fed to the ear as if heard (self-hearing), then the
   next slot. Stops at a space, silence, or the word-length limit.
4. **Self-hearing is not surprise.** Hearing its own voice should not look like new input
   (a corollary-discharge signal): while self-hearing, learning is off in tests and the
   surprise from its own letter is not counted.

## Scoring (quick test)
- next-letter: share of cues where the first spoken letter is the word's next letter (chance ~1/27).
- spelled: share of a word's remaining letters said in the right order.
- Reported next to recall, per word count (30 + 100 words, two brains).

## Risks
- The mouth may echo the cue's last letter instead of predicting (watch next-letter vs echo rate).
- Self-hearing loops can run away (same letter repeated): the length limit and the usual
  brakes (settle_cap) should stop it; measure repeats.
