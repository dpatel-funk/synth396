// Biquad.h  -- STAGE 6.  Doc section: "Level, mix and tone".  Source: W3C Audio EQ Cookbook (low-pass).
// Test: make stage N=6     Plot: DspLab test 1
//   w0 = 2 pi f0/fs     alpha = sin(w0)/(2Q)
//   b0 = b2 = (1 - cos w0)/2     b1 = 1 - cos w0
//   a0 = 1 + alpha     a1 = -2 cos w0     a2 = 1 - alpha        (divide every coefficient by a0)
// Run it as:  y = b0 x + z1;  z1 = b1 x - a1 y + z2;  z2 = b2 x - a2 y
#pragma once
#include "Common.h"

namespace dsp {

class BiquadLowpass {
 public:
  void set(float f0, float q, float fs) { (void)f0; (void)q; (void)fs; /* TODO: keep f0 below fs/2 */ }
  void reset() { /* TODO */ }
  float process(float x) { return x; /* TODO */ }

 private:
  // TODO: 5 coefficients, 2 memory values
};

// Reference for the tests (given).  A digital filter built with the bilinear transform
// squeezes the frequency axis, so the textbook ratio f/f0 becomes tan(pi f/fs)/tan(pi f0/fs):
//   |H| = 1 / sqrt(1 + ratio^4)
inline float butterworthMagnitude(float f, float f0, float fs) {
  const float r = tanf(kPi * f / fs) / tanf(kPi * f0 / fs);
  return 1.0f / sqrtf(1.0f + r * r * r * r);
}

}  // namespace dsp
