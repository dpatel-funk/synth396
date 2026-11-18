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
  void set(float feedback, float damping) { f = clampf(feedback, 0.0f, 0.99f); d = clampf(damping, 0.0f, 0.99f); }
  void clear() { i = 0; s = 0.0f; for (int j = 0; j < N; j++) {buff[j] = 0.0f;} }
  float process(float x) { float out = buff[i]; s = (1 - d)*out + d*s; buff[i] = x + f*s; if (i+1==N) {i=0;} else {i++;} return out; }

 private:
  float f = 0.0f, d = 0.0f, s = 0.0f;
  int i = 0;
  float buff[N] = {};
};

template <int M>
class Allpass {
 public:
  void set(float gr) { g = clampf(gr, 0.0f, 0.999f); }
  void clear() { i = 0; for (int j=0; j<M; j++) {buff[j] = 0.0f;} }
  float process(float x) { float old = buff[i]; float v = x + g*old; buff[i] = v; if (i+1==M) {i=0;} else {i++;} return old - g*v; }

 private:
  float buff[M] = {};
  int i = 0;
  float g = 0.5f;
};

inline float combFeedbackForT60(int n, float t60, float fs) { float g = powf(10.0f, -3.0f * n / (fs * t60)); return g; }

class Reverb {
 public:
  Reverb() { set(0.84f, 0.2f); } 
  void set(float feedback, float damping) { a1.set(0.5); a2.set(0.5); c1.set(feedback, damping); c2.set(feedback, damping);
	c3.set(feedback, damping); c4.set(feedback, damping); }
  void clear() { c1.clear(); c2.clear(); c3.clear(); c4.clear(); a1.clear(); a2.clear(); }
  float process(float x) {
	float sum = (c1.process(x) + c2.process(x) + c3.process(x) + c4.process(x)) / 4;
       return a2.process(a1.process(sum));
  }
 private:
  Comb<1116> c1; Comb<1188> c2; Comb<1277> c3; Comb<1356> c4;
  Allpass<556> a1; Allpass<441> a2;
};

}  // namespace dsp
