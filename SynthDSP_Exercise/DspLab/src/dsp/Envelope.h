// Envelope.h  -- STAGE 4.  Doc section: "Synth voice" (ADSR).
// Test: make stage N=4     Plot: DspLab test 5
// A small state machine: Idle, Attack, Decay, Sustain, Release.
//   attack:  e rises to 1 in A seconds        decay: e falls to S in D seconds
//   release: e falls to 0 in R seconds, starting from WHEREVER e was at note-off
// Think in "step per sample" = (distance to travel) / (seconds * fs).
#pragma once
#include "Common.h"

namespace dsp {

class Adsr {
 public:
  void set(float a, float d, float s, float r, float fs) { (void)a; (void)d; (void)s; (void)r; (void)fs; /* TODO */ }
  void noteOn() { /* TODO */ }
  void noteOff() { /* TODO */ }
  bool active() const { return false; /* TODO */ }
  float next() { return 0.0f; /* TODO */ }

 private:
  // TODO: add your state here
};

}  // namespace dsp
