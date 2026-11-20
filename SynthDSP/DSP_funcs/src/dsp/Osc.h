// Osc.h  -- STAGE 3.  Doc section: "Synth voice" (oscillator).
// Test: make stage N=3     Plot: DspLab test 4
// Keep a phase phi in 0..1 as private state, advance it by f/fs per sample, wrap at 1.
//   sine: sin(2 pi phi)   saw: 2phi-1   square: +1 if phi<0.5 else -1   triangle: 1 - 4|phi-0.5|
#pragma once
#include "Common.h"

namespace dsp {

enum class Wave { Sine, Saw, Square, Triangle };

class Osc {
 public:
  void setFreq(float hz, float fs) { inc = hz/fs; }
  void reset() { phase = 0.0f; }
  float next(Wave w) { float y = 0.0f; if (w == Wave::Sine) y = sinf(2.0f * kPi * phase); 
			else if (w == Wave::Saw) y = 2.0f * phase - 1.0f; 
			else if (w == Wave::Square && phase<0.5) y = 1.0f;
			else if (w == Wave::Square && phase>=0.5) y = -1;
			else if (w == Wave::Triangle) y = 1.0f - 4 * fabsf(phase - 0.5f);
			phase += inc;
			if (phase >= 1.0f) phase -= 1.0f;
			return y;
		}

 private:
  float phase = 0.0f;
  float inc = 0.0f;
};

}  // namespace dsp
