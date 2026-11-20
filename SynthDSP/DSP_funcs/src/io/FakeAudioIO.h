// FakeAudioIO.h - a pretend audio device for laptop tests (never used on the Teensy).
//
// It makes a sine wave as the "input", calls your callback block by block, and records
// both what it sent in and what came out, so a test can inspect them.
#pragma once
#include <math.h>
#include <vector>
#include "AudioIO.h"

namespace dsp {

class FakeAudioIO : public AudioIO {
 public:
  // TODO (you): clear both recordings and restart the sine at phase 0.
  // Keep the callback and the sine settings. Return true.
  bool begin() override { return false; }

  // TODO (you): remember the callback and the user pointer.
  void setCallback(AudioCallback cb, void *user_) override { (void)cb; (void)user_; }

  // TODO (you): remember frequency, amplitude and the sample rate for the fake input.
  void setSine(float hz, float amp, float fs) { (void)hz; (void)amp; (void)fs; }

  // TODO (you): do `blocks` times: fill kBlockSize input samples from the sine (the phase must
  // CONTINUE from where the last block ended), call the callback, then append that block's input
  // and output to inBuf and outBuf.
  void run(int blocks) { (void)blocks; }

  const std::vector<float> &inputs() const { return inBuf; }
  const std::vector<float> &outputs() const { return outBuf; }

 private:
  std::vector<float> inBuf, outBuf;
  // TODO (you): add what else you need (callback, user pointer, frequency, amplitude, fs, phase).
};

}  // namespace dsp
