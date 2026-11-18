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
  void clear() { for (int i = 0; i < N; i++) {buff[i] = 0; w = 0; }}
  void push(float x) { buff[w] = x; if (w+1 == N) {w = 0;} else {w++;}; }
  float tap(float d) const { 
	int di = (int)d; float frac = d-di;
	int p0; int p1; if (w-1-di < 0) {p0 = w-1-di+N;
	} else {
	  p0 = w-1-di;
	} if (p0 == 0) {p1 = N-1;} else {p1 = p0 - 1;}
	return (buff[p0] + frac * (buff[p1]-buff[p0])); }

 private:
  float buff[N] = {};
  int w = 0;
};

// Echo.   w[n] = b[n-D],  b[n] = x[n] + f*w[n],  y[n] = (1-m)*x[n] + m*w[n]
// clampf f below 1 so it can never run away.
template <int N>
class Echo {
 public:
  void set(float delaySamples, float feedback, float mix) { D = clampf(delaySamples, 1.0f, (float)(N - 2)); f = clampf(feedback, 0.0f, 0.99f); m=mix;}
  float process(float x) { float w = line.tap(D-1); line.push(x + f * w); return ((1-m) * x + m * w); }
  void clear() { line.clear(); }

 private:
  DelayLine<N> line;
  float D = 0.0f, f = 0.0f, m = 0.0f;
};

// Chorus.  d[n] = d0 + depth*sin(2 pi phi),  phi += rate/fs (wrap at 1),  y = (1-m)x + m * x[n - d[n]]
template <int N>
class Chorus {
 public:
  void set(float baseSamples, float depthSamples, float rateHz, float mix, float fs) {
	base = baseSamples; depth = depthSamples; inc = rateHz/fs; m = mix;
  }
  float process(float x) { line.push(x); phi += inc;
	if (phi >= 1.0f) phi -= 1.0f; 
	float d = clampf(base + depth * sinf(2*kPi*phi), 0.0f, (float)N-2);
	float w = line.tap(d); return (1 - m)*x + m*w; 
  }
  void clear() { 
	line.clear(); phi = 0.0f; 
  }

 private:
  DelayLine<N> line;
  float base = 0.0f, depth = 0.0f, inc = 0.0f, m = 0.0f, phi = 0.0f;
};

}  // namespace dsp
