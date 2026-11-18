# SynthDSP

Original, header-only C++ for the guitar synth's DSP blocks. No Teensy Audio library,
no Arduino calls inside `src/dsp/`, so the same code runs on a laptop and on the Teensy.

```
SynthDSP/
  DspLab/                  <- open this folder in Arduino IDE (sketch name = folder name)
    DspLab.ino             <- sends test curves to the Serial Plotter
    src/dsp/
      Common.h             dB<->gain, pot maps, MIDI->Hz, equal-power mix
      Biquad.h             RBJ low-pass (TONE) + Butterworth theory
      Delay.h              DelayLine, Echo (DLY), Chorus (CHO)
      Distortion.h         softClip, hardClip, bitcrush (DST)
      Osc.h                phase-accumulator oscillator (SYNTH)
      Envelope.h           linear ADSR
      Reverb.h             lowpass-feedback combs + allpasses (REV)
      Analysis.h           measureGainDb(): feed a sine, read back the gain
      All.h                includes everything
  tests/
    run_tests.cpp          checks every block against its theory
    Makefile               `make test` on a computer (needs g++)
```

## The plan

**Phase 1 (do this first): write the functions.** Eight stages, easiest first. Every file in
`DspLab/src/dsp/` is a skeleton that compiles, returns dummy values, and has a short formula hint.
Fill in the bodies yourself; the doc "Guitar Synth: DSP Formulas and Sources" explains each formula.

**Phase 2 (after Phase 1): the main body.** An `Engine` class that owns all the blocks, a `Controls`
snapshot of the front panel (6 pots, 8 buttons, footswitch), `setControls()` at control rate and
`process()` at audio rate, parameter smoothing, effect select, and the synth voice. Not built yet.

| Stage | File | What you write | Run on the computer | Breadboard check |
|---|---|---|---|---|
| 1 | Common.h | dB to gain, pot maps, MIDI to Hz, equal-power mix | `make stage N=1` | **B1** (below) |
| 2 | Distortion.h | softClip, hardClip, bitcrush | `make stage N=2` | plot 3 |
| 3 | Osc.h | phase accumulator, four waves | `make stage N=3` | plot 4 |
| 4 | Envelope.h | ADSR state machine | `make stage N=4` | plot 5 |
| 5 | Delay.h | DelayLine, Echo | `make stage N=5` | plot 2 |
| 6 | Biquad.h | low-pass filter (TONE) | `make stage N=6` | plot 1 |
| 7 | Delay.h | Chorus | `make stage N=7` | (numbers only) |
| 8 | Reverb.h | Comb, Allpass, Reverb | `make stage N=8` | plot 6 |

Run `make` commands from the `tests/` folder (needs g++). `make test` runs every stage at once.
Each test line prints your measured value next to the value theory predicts, so a FAIL tells you
what to look at.

## Breadboard checks

- **B1, controls (after stage 1):** wire the pots, buttons and footswitch as in the pin map at the top
  of the LIVE section in `DspLab.ino`, upload, open Serial **Monitor** and send `7`. Turn each pot
  end to end: LEVEL should read -40 to +6 dB, TONE 200 to 8000 Hz, MIX 0 to 1, TIME 20 to 600 ms,
  PITCH -12 to +12 st, SYNTH -40 to 0 dB. Press BTN5-8: each should print its note frequency, and
  PITCH should bend it. The footswitch should toggle FX and the LED. This tests your stage-1 maths
  with real hardware and real pot noise.
- **B2, DSP on the real chip (after stages 2-8):** upload `DspLab` and open the Serial Plotter, with
  `TEST_TO_RUN` set to 1-6. The curves should match what `make test` showed on your computer. Same
  maths, different processor: a difference means a float or timing problem worth finding now.
- **B3, live audio:** not possible yet. It needs code for the audio shield chip (I2S plus codec setup).
  That is the Phase 2 decision point.

```
DspLab/DspLab.ino      plot tests 1-6 and the LIVE readout 7
DspLab/src/dsp/        your code (skeletons now); Analysis.h and All.h are given
tests/run_tests.cpp    checks each block against theory
```

Tips: the Arduino IDE compiles `.cpp` files inside `src/`; headers are fine anywhere in the sketch
folder. Keep `src/dsp/` free of Arduino calls so the same code runs on your laptop and on the Teensy.
The pin numbers and "pressed = LOW" wiring come from the earlier controls sketch; compare them with
your actual wiring before trusting B1.
