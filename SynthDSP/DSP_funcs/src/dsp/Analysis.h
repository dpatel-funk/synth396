// Analysis.h - helpers to measure what your DSP actually does.
#pragma once
#include "Common.h"

namespace dsp {

// Feed a sine of `freq` through `process` (a callable float->float) and return the
// output/input RMS ratio in dB, measured after a settling period.
template <class F>
float measureGainDb(F &&process, float freq, float fs, int settle = 4000, int measure = 4000) {
  double inSq = 0, outSq = 0;
  for (int n = 0; n < settle + measure; n++) {
    const float x = sinf((float)(2.0 * 3.14159265358979 * (double)freq * (double)n / (double)fs));
    const float y = process(x);
    if (n >= settle) { inSq += (double)x * x; outSq += (double)y * y; }
  }
  return 10.0f * (float)log10(outSq / inSq);
}

}  // namespace dsp
