// Engine.h - STAGE 13: owns every block and runs the signal chain.
//
//   control rate  setControls(c)   panel snapshot -> targets          (from loop())
//   audio rate    process(x)       one sample in, one sample out      (from the audio callback)
//
//   FX on : y = levelGain * tone( dryG*x + wetG*fx(x) ) + synthGain * voice
//   FX off: y = levelGain * x                           + synthGain * voice
//   (dryG, wetG) = equalPowerMix(mix): the ENGINE does the mixing, so Echo and Chorus are
//   set to mix = 1 (fully wet) and Reverb / softClip are wet-only already.
//
// NOTE: this object is big (~150 KB). Declare it `static` / global, never as a local variable.
#pragma once
#include "../dsp/All.h"
#include "Controls.h"
#include "Smoother.h"
#include "EffectSelect.h"
#include "Voice.h"

namespace dsp {

class Engine {
 public: 
  // full reset
  void begin(float fs_) { 
	fs = fs_;
	levelGain.set(kSmooth, fs); synthGain.set(kSmooth, fs); mix.set(kSmooth, fs); 
	toneHz.set(kSmooth, fs); timeMs.set(kSmooth, fs); bend.set(kSmooth, fs);
	echo.clear();
	chorus.clear();
	reverb.clear();
	tone.reset();
	voice = Voice();
	voice.set(0.01f, 0.1f, 0.7f, 0.2f, fs);
	sel = EffectSelect();
	first = true;
	curKey = -1;
	for (int j = 0; j<4; j++) {prevKey[j] = false;}
  }
	

  void setControls(const Controls &c) {
	sel.update(c);
	Params p = mapControls(c);
	for (int i = 0; i < 4; i++) {
  	  bool key = c.btn[4 + i];
  	  if (prevKey[i]==false && key==true) { voice.noteOn(notes[i]); curKey = i; }
  	  if (prevKey[i]==true && key==false && i==curKey) { voice.noteOff(); curKey = -1; }
  	  prevKey[i] = key;
	}
	levelGain.setTarget(dbToGain(p.levelDb));
	synthGain.setTarget(dbToGain(p.synthDb));
	mix.setTarget(p.mix);
	toneHz.setTarget(p.toneHz);
	timeMs.setTarget(p.timeMs);
	bend.setTarget(p.pitchSemis);
	if (first) { 
	  levelGain.reset(dbToGain(p.levelDb)); synthGain.reset(dbToGain(p.synthDb)); 
	  mix.reset(p.mix); toneHz.reset(p.toneHz); timeMs.reset(p.timeMs); bend.reset(p.pitchSemis);
	  first = false;
	}
  }
	
  float process(float x) {
	float lg = levelGain.next();
  	float sg = synthGain.next();
  	float m = mix.next(); float hz = toneHz.next(); float tms = timeMs.next(); float b = bend.next();
	Fx fx = sel.currentFx();
	float chain = x;
	if (sel.fxOn()) {
	  float wet = 0.0f;
	  switch (fx) {
	    case Fx::Dly:
  		  echo.set(tms * fs / 1000.0f, 0.4f, 1.0f);
  		  wet = echo.process(x);
  		  break;
	    case Fx::Dst:
		  wet = softClip(x, 1 + (tms-20)/29);
		  break;
	    case Fx::Cho:
		  chorus.set(15.0f * fs / 1000.0f, 3.0f * fs / 1000.0f, 200.0f / tms, 1.0f, fs);
  		  wet = chorus.process(x);
		  break;
	    case Fx::Rev: {
		  float u = (tms - 20.0f) / 580.0f;
  		  reverb.set(linMap(u, 0.7f, 0.95f), 0.2f);
  		  wet = reverb.process(x);
		  break;
	    }
	  }
	  float dryG, wetG;
	  equalPowerMix(m, dryG, wetG);
	  tone.set(hz, 0.7071f, fs);
	  chain = tone.process(dryG * x + wetG * wet);
  	}
	float v = voice.next(b);
  return lg * chain + sg * v;
  }

  // given: a block is just process() in a loop
  void processBlock(const float *in, float *out, int n) { for (int i = 0; i < n; i++) out[i] = process(in[i]); }

 private:
  // design constants (my proposals; tune by ear later)
  //   smoothing time constant ................ kSmooth = 0.01 s
  //   synth envelope ......................... A 0.01 s, D 0.1 s, S 0.7, R 0.2 s
  //   tone filter Q .......................... 0.7071
  //   DLY : delay = timeMs*fs/1000 samples, feedback 0.4, echo mix 1
  //   CHO : base = 15 ms, depth = 3 ms (in samples), rateHz = 200/timeMs, chorus mix 1
  //   DST : drive = 1 + (timeMs - 20)/29   -> softClip(x, drive)
  //   REV : feedback = linMap((timeMs-20)/580, 0.7, 0.95), damping 0.2
  static constexpr float kSmooth = 0.01f;
  const int notes[4] = {60, 62, 64, 67};   // BTN5..BTN8 = C D E G

  float fs = 44100.0f;
  bool  first = true;

  EffectSelect sel;
  Smoother levelGain, synthGain, mix, toneHz, timeMs, bend;

  Echo<26460>   echo;
  Chorus<2048>  chorus;
  Reverb        reverb;
  BiquadLowpass tone;
  Voice         voice;

  bool prevKey[4] = {false, false, false, false};
  int  curKey = -1;
};

}  // namespace dsp
