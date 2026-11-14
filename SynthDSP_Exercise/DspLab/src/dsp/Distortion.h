// Distortion.h  -- STAGE 2.  Doc section: "Distortion".  Inputs/outputs are -1..1.
// Test: make stage N=2
#pragma once
#include "Common.h"

namespace dsp {

// tanh(drive*x) / tanh(drive).  Dividing keeps f(1) = 1 for any drive. (tanhf is in <math.h>)
inline float softClip(float x, float drive) { return (tanhf(drive*x)/tanhf(drive));}

// Multiply by drive, then chop at +-1 (clampf is given).
inline float hardClip(float x, float drive) { return clampf(x * drive, -1.0f, 1.0f); }

// Round to 2^(bits-1) levels per polarity.  Hint: scale up, roundf, scale back down.
inline float bitcrush(float x, int bits) { float scale = powf(2.0f, bits-1.0f); float rnd = roundf(scale*x); return (rnd/scale); }

} 
