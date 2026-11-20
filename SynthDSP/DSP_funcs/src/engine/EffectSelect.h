// EffectSelect.h - STAGE 11: button presses -> which effect, and bypass on/off.
//
// update() is called once per control tick with the latest panel snapshot.
// It reacts to PRESS EDGES (up -> down), never to a button merely being held.
//   BTN1..BTN4 (btn[0..3]) : choose DLY, CHO, DST, REV. Last press wins.
//   Footswitch (fsw)       : each press edge toggles fxOn.
// btn[4..7] (the synth notes) are not this class's business.

#pragma once
#include "Controls.h"

namespace dsp {

enum class Fx { Dly = 0, Cho = 1, Dst = 2, Rev = 3 };

class EffectSelect {
 public:
  // update the selection / toggle, then remember this tick's states for next time.
  // If two buttons have an edge on the same tick, the higher-numbered one wins.
  void update(const Controls &c) { 
     for (int j=0; j<4; j++) {
	if(prevBtn[j] == false && c.btn[j] == true) {
	prevBtn[j] = c.btn[j]; cur = static_cast<Fx>(j);
	} else {
		prevBtn[j] = c.btn[j];
	}
      }
      if (prevFsw == false && c.fsw == true) {prevFsw = c.fsw; on = !on;} else { prevFsw = c.fsw; }
  }

  Fx   currentFx() const { return cur; }
  bool fxOn() const { return on; }

 private:
  Fx   cur = Fx::Dly;
  bool on  = false;
  bool prevBtn[4] = {false, false, false, false};
  bool prevFsw = false;

};

}  // namespace dsp
