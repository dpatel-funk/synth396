// Reverb.h  -- STAGE 8.  Doc section: "Reverb".
// Test: make stage N=8     Plot: DspLab test 6
//   Comb (lowpass-feedback):  out = buf[i];  s = (1-d)*out + d*s;  buf[i] = x + f*s;  advance i
//   Allpass:                  old = buf[i];  v = x + g*old;  buf[i] = v;  advance i;  return old - g*v  (g = 0.5)
//   Feedback for a decay of T60 seconds with a comb of N samples:  g = 10^(-3N / (fs*T60))
//   Reverb: average a few combs fed with the same input, then pass through two allpasses in series.
//           Try comb lengths 1116, 1188, 1277, 1356 and allpass lengths 556, 441 (typical Freeverb
//           tuning values; tune by ear).  set(feedback, damping) goes to every comb.
#pragma once
#include "Common.h"

namespace dsp {

template <int N>
class Comb {
 public:
  void set(float feedback, float damping) { (void)feedback; (void)damping; /* TODO */ }
  void clear() { /* TODO */ }
  float process(float x) { (void)x; return 0.0f; /* TODO */ }

 private:
  // TODO
};

template <int M>
class Allpass {
 public:
  void set(float g) { (void)g; /* TODO */ }
  void clear() { /* TODO */ }
  float process(float x) { (void)x; return 0.0f; /* TODO */ }

 private:
  // TODO
};

inline float combFeedbackForT60(int n, float t60, float fs) { (void)n; (void)t60; (void)fs; return 0.0f; /* TODO */ }

class Reverb {
 public:
  void set(float feedback, float damping) { (void)feedback; (void)damping; /* TODO */ }
  void clear() { /* TODO */ }
  float process(float x) { return x; /* TODO */ }

 private:
  // TODO: 4 combs and 2 allpasses
};

}  // namespace dsp
