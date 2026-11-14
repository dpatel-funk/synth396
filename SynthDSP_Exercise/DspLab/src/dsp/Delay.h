// Delay.h  -- STAGE 5 (DelayLine, Echo) and STAGE 7 (Chorus).  Doc sections: "Delay", "Chorus and flanger".
// Tests: make stage N=5   then   make stage N=7     Plot: DspLab test 2
#pragma once
#include "Common.h"

namespace dsp {

// Circular buffer of N floats.
// push(x) stores the newest sample.  tap(d) returns the sample pushed d pushes ago
// (tap(0) = newest).  For fractional d, linearly interpolate between the two neighbours:
//   a + (b - a) * frac
template <int N>
class DelayLine {
 public:
  void clear() { /* TODO */ }
  void push(float x) { (void)x; /* TODO */ }
  float tap(float d) const { (void)d; return 0.0f; /* TODO */ }

 private:
  // TODO: buffer, write index
};

// Echo.   w[n] = b[n-D],  b[n] = x[n] + f*w[n],  y[n] = (1-m)*x[n] + m*w[n]
// Careful: which tap() gives b[n-D] BEFORE you push b[n]?  (The test will tell you if it is off by one.)
// Clamp f below 1 so it can never run away.
template <int N>
class Echo {
 public:
  void set(float delaySamples, float feedback, float mix) { (void)delaySamples; (void)feedback; (void)mix; /* TODO */ }
  float process(float x) { (void)x; return 0.0f; /* TODO */ }
  void clear() { /* TODO */ }

 private:
  // TODO: a DelayLine<N> plus your parameters
};

// Chorus.  d[n] = d0 + depth*sin(2 pi phi),  phi += rate/fs (wrap at 1),  y = (1-m)x + m * x[n - d[n]]
template <int N>
class Chorus {
 public:
  void set(float baseSamples, float depthSamples, float rateHz, float mix, float fs) {
    (void)baseSamples; (void)depthSamples; (void)rateHz; (void)mix; (void)fs; /* TODO */
  }
  float process(float x) { (void)x; return 0.0f; /* TODO */ }
  void clear() { /* TODO */ }

 private:
  // TODO
};

}  // namespace dsp
