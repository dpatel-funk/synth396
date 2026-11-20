// Smoother.h - STAGE 10: glide a parameter toward its target, one small step per audio sample.
//
//   p += k * (target - p),   k = 1 - exp(-1 / (tau * fs))
//
// setTarget() is called at control rate; next() is called once per audio sample.
#pragma once
#include <math.h>

namespace dsp {

class Smoother {
 public:
  void set(float timeSeconds, float fs) { k = 1 - expf(-1/(timeSeconds * fs)); }

  // the first sound does not glide up from 0).
  void reset(float x) { p = x; target = x; }

  void setTarget(float x) { target = x; }

  float next() { p += k * (target - p); return p; }

 private:
  float k = 0.0f, p = 0.0f, target = 0.0f;
};

}  // namespace dsp
