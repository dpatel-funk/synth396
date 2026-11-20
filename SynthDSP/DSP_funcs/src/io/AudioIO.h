// AudioIO.h - the boundary between the engine and wherever audio comes from.
//
// The engine only ever sees "a block of input samples in, a block of output samples out".
// It never knows if the samples came from a test, the Teensy Audio library, or hand-written I2S.
//
// A callback receives n input samples and must fill n output samples. n is kBlockSize (128).
// `user` is a pointer you hand over when registering, so a plain function can find your object
// (for example the Engine) without std::function.
#pragma once

namespace dsp {

constexpr int kBlockSize = 128;   // samples per block: about 2.9 ms at 44.1 kHz

typedef void (*AudioCallback)(const float *in, float *out, int n, void *user);

// TODO (you): make both functions PURE virtual (replace the {...} bodies with `= 0;`).
// A pure virtual function has no body here: every class that wants to be an AudioIO MUST provide one.
class AudioIO {
 public:
  virtual ~AudioIO() {}
  virtual bool begin() { return false; }                                  // start; true if it worked
  virtual void setCallback(AudioCallback cb, void *user) { (void)cb; (void)user; }
};

}  // namespace dsp
