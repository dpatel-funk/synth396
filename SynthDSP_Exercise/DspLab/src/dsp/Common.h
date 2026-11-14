// Common.h  -- STAGE 1.  Doc section: "Pots to parameters" and "Level, mix, tone".
// Write each function body. Test: make stage N=1
#pragma once
#include <math.h>

namespace dsp {

constexpr float kPi = 3.14159265358979f;

inline float clampf(float x, float lo, float hi) { return x < lo ? lo : (x > hi ? hi : x); }  // given

// dB -> linear gain.            
inline float dbToGain(float db) { (void)db; return (powf(10, db/20));}

// linear gain -> dB.            
inline float gainToDb(float g) { (void)g; return (20.0f * log10f(g));}

// u in 0..1 -> lo..hi, equal pot travel = equal RATIO. 
inline float expMap(float u, float lo, float hi) { return lo * powf(hi/lo, u);}

// u in 0..1 -> lo..hi in a straight line.
inline float linMap(float u, float lo, float hi) { return lo + u * (hi - lo);}

// MIDI note + bend (semitones) -> Hz.   
inline float midiToHz(float note, float bendSemitones = 0.0f) { float expo = note - 69.0f + bendSemitones; return 440.0f * powf(2, expo/12.0f);}

// Equal-power dry/wet gains for mix m in 0..1. 
inline void equalPowerMix(float m, float &dryGain, float &wetGain) {
  float angle = (kPi/4) * m; dryGain = cosf(angle); wetGain = sinf(angle);
}

}  // namespace dsp
