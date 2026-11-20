// Controls.h - STAGE 9: one snapshot of the whole front panel, and its conversion to parameters.
//
// Control rate (loop(), a few hundred times a second) fills a Controls, calls mapControls(),
// and hands the resulting Params to the engine. Nothing here touches Arduino functions.
#pragma once
#include "../dsp/Common.h"

namespace dsp {

// Raw panel state. pot[i] is already 0..1 (from analogRead / 1023 in loop(), or set by a test).
// Index order matches the panel: 0 LEVEL, 1 TONE, 2 MIX, 3 TIME, 4 PITCH, 5 SYNTH.
struct Controls {
  float pot[6] = {0, 0, 0, 0, 0, 0};
  bool  btn[8] = {false, false, false, false, false, false, false, false};  // true = pressed
  bool  fsw    = false;                                                     // true = pressed
};

// The same information in the units the DSP wants.
struct Params {
  float levelDb    = 0;  // dB
  float toneHz     = 0;  // Hz
  float mix        = 0;  // 0..1
  float timeMs     = 0;  // ms
  float pitchSemis = 0;  // semitones
  float synthDb    = 0;  // dB
};


//   LEVEL -40..+6 dB (lin)   TONE 200..8000 Hz (exp)   MIX 0..1 (lin)
//   TIME 20..600 ms (exp)    PITCH -12..+12 st (lin)   SYNTH -40..0 dB (lin)
// Also: a pot value outside 0..1 must behave like the nearest end (clampf first).

inline Params mapControls(const Controls &c) {
  Params P;
  P.levelDb 	= linMap(clampf(c.pot[0], 0.0f, 1.0f), -40.0f, 6.0f);
  P.toneHz 	= expMap(clampf(c.pot[1], 0.0f, 1.0f), 200.0f, 8000.0f);
  P.mix 	= linMap(clampf(c.pot[2], 0.0f, 1.0f), 0.0f, 1.0f);
  P.timeMs	= expMap(clampf(c.pot[3], 0.0f, 1.0f), 20.0f, 600.0f);
  P.pitchSemis	= linMap(clampf(c.pot[4], 0.0f, 1.0f), -12.0f, 12.0f);
  P.synthDb	= linMap(clampf(c.pot[5], 0.0f, 1.0f), -40.0f, 0.0f);
  return P;
}

}  // namespace dsp
