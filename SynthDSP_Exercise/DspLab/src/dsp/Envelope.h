// Envelope.h  -- STAGE 4.  Doc section: "Synth voice" (ADSR).
// Test: make stage N=4     Plot: DspLab test 5
// A small state machine: Idle, Attack, Decay, Sustain, Release.
//   attack:  e rises to 1 in A seconds        decay: e falls to S in D seconds
//   release: e falls to 0 in R seconds, starting from WHEREVER e was at note-off

#pragma once
#include "Common.h"

namespace dsp {

class Adsr {
 public:
  void set(float a, float d, float s, float r, float fs) { attackInc = 1.0f / (a * fs); decayInc = (1.0f - s) / (d * fs); sustain = s; releaseSamples = r * fs;}
  void noteOn() { stage = Stage::Attack; }
  void noteOff() { 
	releaseInc = e / releaseSamples;
        stage = Stage::Release; 
  }
  bool active() const { return stage != Stage::Idle; }
  float next() { if (stage == Stage::Attack) { 
		    e = e + attackInc; 
		    if (e >= 1.0f) { 
			e = 1; stage = Stage::Decay; 
		    } 
		} else if (stage == Stage::Decay) {
			e = e - decayInc;
			if (e <= sustain) { e = sustain; stage = Stage::Sustain; } 
		} else if (stage == Stage::Release) {
  			e = e - releaseInc;
  			if (e <= 0.0f) { 
			    e = 0.0f; stage = Stage::Idle; 
			}
		}
		return e; 
	    }

 private:
  enum class Stage { Idle, Attack, Decay, Sustain, Release };
  Stage stage = Stage::Idle;
  float e = 0.0f;
  float attackInc = 0.0f;
  float decayInc = 0.0f;
  float sustain = 0.0f;
  float releaseSamples = 0.0f;
  float releaseInc = 0.0f;
};

}  // namespace dsp
