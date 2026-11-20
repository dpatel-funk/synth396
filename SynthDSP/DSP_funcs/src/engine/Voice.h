// Voice.h - STAGE 12: one monophonic synth voice = oscillator x envelope.
//
//   output = osc(freq) * envelope
//   freq   = midiToHz(note, bendSemitones), recomputed EVERY sample so the PITCH pot
//            works while a note is held.
//
// noteOn(note) while another note is sounding just retunes and retriggers (last press wins).
// noteOff() starts the envelope's release; the oscillator keeps running under the fade.
#pragma once
#include "../dsp/Common.h"
#include "../dsp/Osc.h"
#include "../dsp/Envelope.h"

namespace dsp {

class Voice {
 public:
  void set(float a, float d, float s, float r, float fs_) { env.set(a, d, s, r, fs_); 
	fs = fs_; }

  void noteOn(int note_) { note = note_; env.noteOn(); }

  void noteOff() { env.noteOff(); }

  bool active() const { return env.active(); }

  // TODO (you): called once per audio sample. Set the oscillator's frequency from the note and
  // the bend, then return oscillator sample * envelope sample.
  float next(float bendSemitones) { 
	osc.setFreq(midiToHz(note, bendSemitones), fs); 
	return (osc.next(wave) * env.next()); 
  } 

 private:
  Osc  osc;
  Adsr env;
  Wave wave = Wave::Sine;
  int note = 0;
  float fs = 0.0f;
};

}  // namespace dsp
