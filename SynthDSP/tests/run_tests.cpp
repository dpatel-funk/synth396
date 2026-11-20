// Native test program: checks every DSP block against its theory.
// Build and run on a computer:   make test      (or ./run_tests.sh)
#include <stdio.h>
#include <stdlib.h>
#include "../DSP_funcs/src/dsp/All.h"
#include "../DSP_funcs/src/engine/Controls.h"
#include "../DSP_funcs/src/engine/Smoother.h"
#include "../DSP_funcs/src/engine/EffectSelect.h"
#include "../DSP_funcs/src/engine/Voice.h"
#include "../DSP_funcs/src/engine/Engine.h"
#include "../DSP_funcs/src/io/AudioIO.h"
#include "../DSP_funcs/src/io/FakeAudioIO.h"
using namespace dsp;

static int fails = 0;
static int stage = 0;                       // 0 = run everything
static bool want(int k) { return stage == 0 || stage == k; }
static void check(bool ok, const char *name, double got, double want) {
  printf("%s  %-44s got %-12.5g want %-12.5g\n", ok ? "PASS" : "FAIL", name, got, want);
  if (!ok) fails++;
}
static bool near(double a, double b, double tol) { return fabs(a - b) <= tol; }

struct CbCount { int calls = 0; bool all128 = true; };
static void countCb(const float *in, float *out, int n, void *user) {
  CbCount *k = static_cast<CbCount *>(user); k->calls++; if (n != kBlockSize) k->all128 = false;
  for (int i = 0; i < n; i++) out[i] = in[i];
}
static void engineCb(const float *in, float *out, int n, void *user) { static_cast<Engine *>(user)->processBlock(in, out, n); }

int main(int argc, char **argv) {
  if (argc > 1) stage = atoi(argv[1]);
  const float fs = 44100.0f;

  // dB <-> gain, pitch
  if (want(1)) {
  check(near(dbToGain(-6.0206f), 0.5, 1e-3), "dbToGain(-6.02 dB) = 0.5", dbToGain(-6.0206f), 0.5);
  check(near(gainToDb(0.5f), -6.0206, 1e-3), "gainToDb(0.5) = -6.02 dB", gainToDb(0.5f), -6.0206);
  check(near(gainToDb(dbToGain(-13.0f)), -13.0, 1e-3), "round trip: -13 dB comes back", gainToDb(dbToGain(-13.0f)), -13.0);
  check(near(midiToHz(69), 440.0, 1e-3), "midiToHz(69) = 440", midiToHz(69), 440.0);
  check(near(midiToHz(60), 261.626, 1e-2), "midiToHz(60) = 261.63", midiToHz(60), 261.626);
  check(near(midiToHz(60, 12), 523.251, 1e-2), "bend +12 doubles the frequency", midiToHz(60, 12), 523.251);
  check(near(expMap(0.0f, 200, 8000), 200.0, 1e-3),  "expMap(0)   = lo",            expMap(0.0f, 200, 8000), 200.0);
  check(near(expMap(1.0f, 200, 8000), 8000.0, 1e-2), "expMap(1)   = hi",            expMap(1.0f, 200, 8000), 8000.0);
  check(near(expMap(0.5f, 200, 8000), 1264.911, 1e-2), "expMap(0.5) = geometric mean", expMap(0.5f, 200, 8000), 1264.911);
  check(near(linMap(0.0f, -40, 6), -40.0, 1e-4),     "linMap(0)   = lo",            linMap(0.0f, -40, 6), -40.0);
  check(near(linMap(1.0f, -40, 6), 6.0, 1e-4),       "linMap(1)   = hi",            linMap(1.0f, -40, 6), 6.0);
  check(near(linMap(0.25f, -40, 6), -28.5, 1e-4),    "linMap(0.25) = -28.5",        linMap(0.25f, -40, 6), -28.5);
  { float d, w; equalPowerMix(0.5f, d, w);
    check(near(d * d + w * w, 1.0, 1e-5), "equal-power mix: dry^2+wet^2 = 1", d * d + w * w, 1.0); }
  { float d, w; equalPowerMix(0.0f, d, w);
    check(near(d, 1.0, 1e-5) && near(w, 0.0, 1e-5), "equal-power mix m=0: all dry (1, 0)", w, 0.0); }
  { float d, w; equalPowerMix(1.0f, d, w);
    check(near(d, 0.0, 1e-5) && near(w, 1.0, 1e-5), "equal-power mix m=1: all wet (0, 1)", d, 0.0); }
  { float d, w; equalPowerMix(0.5f, d, w);
    check(near(d, 0.70711, 1e-4) && near(w, 0.70711, 1e-4), "equal-power mix m=0.5: both 0.7071", d, 0.70711); }
  }

  // Biquad low-pass vs 2nd-order Butterworth theory (Q = 0.7071), f0 = 1 kHz
  if (want(6)) {
  const float freqs[] = {100, 500, 1000, 2000, 5000, 10000};
  for (float f : freqs) {
    BiquadLowpass lp; lp.set(1000.0f, 0.70710678f, fs);
    float meas = measureGainDb([&](float x) { return lp.process(x); }, f, fs);
    float theo = 20.0f * log10f(butterworthMagnitude(f, 1000.0f, fs));
    char name[64]; snprintf(name, sizeof name, "LPF 1kHz gain at %g Hz (dB)", f);
    check(near(meas, theo, 0.1), name, meas, theo);
  }
  }

  // Echo: impulse in, expect 1*m at D samples, then f*m at 2D ...
  if (want(5)) {
  {
    Echo<2048> e; const int D = 100; e.set(D, 0.5f, 1.0f);
    float y[400] = {};
    for (int n = 0; n < 400; n++) y[n] = e.process(n == 0 ? 1.0f : 0.0f);
    check(near(y[0], 0.0, 1e-6), "echo: dry gone at mix=1 (n=0)", y[0], 0.0);
    check(near(y[D], 1.0, 1e-5), "echo: first repeat at n=D, amplitude 1", y[D], 1.0);
    check(near(y[2 * D], 0.5, 1e-5), "echo: second repeat at 2D, amplitude f", y[2 * D], 0.5);
    check(near(y[3 * D], 0.25, 1e-5), "echo: third repeat at 3D, amplitude f^2", y[3 * D], 0.25);
  }
  }

  // Delay line interpolation: ramp input, fractional tap
  if (want(5)) {
  {
    DelayLine<64> dl; for (int n = 0; n < 40; n++) dl.push((float)n);
    check(near(dl.tap(0), 39.0, 1e-5), "delay line tap(0) = newest", dl.tap(0), 39.0);
    check(near(dl.tap(10.5f), 28.5, 1e-4), "delay line tap(10.5) interpolates", dl.tap(10.5f), 28.5);
  }
  }

  // Chorus with zero depth and mix 1 is a plain delay
  if (want(7)) {
  {
    Chorus<2048> c; c.set(50.0f, 0.0f, 0.5f, 1.0f, fs);
    float y[100] = {};
    for (int n = 0; n < 100; n++) y[n] = c.process(n == 0 ? 1.0f : 0.0f);
    check(near(y[50], 1.0, 1e-5), "chorus (depth 0) delays impulse by d0", y[50], 1.0);
  }
  }

  // Distortion
  if (want(2)) {
  check(near(softClip(1.0f, 5.0f), 1.0, 1e-6), "softClip(1, drive) = 1", softClip(1.0f, 5.0f), 1.0);
  check(near(softClip(-1.0f, 5.0f), -1.0, 1e-6), "softClip is odd: f(-1) = -1", softClip(-1.0f, 5.0f), -1.0);
  { float m = 0; for (int i = -100; i <= 100; i++) m = fmaxf(m, fabsf(softClip(i / 50.0f, 20.0f)));
    check(m <= 1.0001, "softClip never exceeds 1 (inputs to +-2)", m, 1.0); }
  check(near(hardClip(0.8f, 3.0f), 1.0, 1e-6), "hardClip(0.8, 3) clips to 1", hardClip(0.8f, 3.0f), 1.0);
  // 2 bits -> 2 levels per polarity -> step of 0.5
  check(near(bitcrush(0.2f, 2), 0.0, 1e-6), "bitcrush 2 bits: 0.2 -> 0", bitcrush(0.2f, 2), 0.0);
  check(near(bitcrush(0.3f, 2), 0.5, 1e-6), "bitcrush 2 bits: 0.3 -> 0.5", bitcrush(0.3f, 2), 0.5);
  check(near(bitcrush(0.8f, 2), 1.0, 1e-6), "bitcrush 2 bits: 0.8 -> 1", bitcrush(0.8f, 2), 1.0);
  }

  // Oscillator: count rising zero crossings in 1 s -> frequency
  if (want(3)) {
  {
    Osc o; o.setFreq(440.0f, fs); float prev = 0; int cross = 0;
    for (int n = 0; n < (int)fs; n++) { float y = o.next(Wave::Sine); if (prev < 0 && y >= 0) cross++; prev = y; }
    check(cross >= 439 && cross <= 441, "sine 440 Hz: crossings in 1 s", cross, 440);
  }
  {
    Osc o; o.setFreq(100.0f, fs); float mn = 9, mx = -9;
    for (int n = 0; n < 2000; n++) { float y = o.next(Wave::Triangle); mn = fminf(mn, y); mx = fmaxf(mx, y); }
    check(near(mn, -1.0, 0.01) && near(mx, 1.0, 0.01), "triangle spans -1..+1", mx - mn, 2.0);
  }
  }

  // Envelope: reaches 1 after A, sits at S, falls to 0 after R
  if (want(4)) {
  {
    Adsr a; a.set(0.01f, 0.02f, 0.5f, 0.05f, fs); a.noteOn();
    float e = 0; for (int n = 0; n < (int)(0.01f * fs); n++) e = a.next();
    check(near(e, 1.0, 0.01), "ADSR: e = 1 at end of attack", e, 1.0);
    for (int n = 0; n < (int)(0.05f * fs); n++) e = a.next();
    check(near(e, 0.5, 1e-4), "ADSR: sustain level S", e, 0.5);
    a.noteOff();
    for (int n = 0; n < (int)(0.05f * fs) + 2; n++) e = a.next();
    check(near(e, 0.0, 1e-4) && !a.active(), "ADSR: back to 0 after release", e, 0.0);
  }
  }

  // Comb feedback for T60: echo 1 has amplitude 1, echo k has g^(k-1)
  if (want(8)) {
  {
    Reverb r2; float peak1 = 0;
    for (int n = 0; n < 5000; n++) { float y = r2.process(n == 0 ? 1.0f : 0.0f);
    if (n > 0) peak1 = fmaxf(peak1, fabsf(y)); }
    check(peak1 > 0.01f, "reverb: echoes appear after the first sample", peak1, 0.01);

    const int N = 1116; const float T60 = 2.0f;
    float g = combFeedbackForT60(N, T60, fs);
    Comb<1116> c; c.set(g, 0.0f);
    float peak = 0; int k = 0;
    for (int n = 0; n < (int)(T60 * fs); n++) {
      float y = c.process(n == 0 ? 1.0f : 0.0f);
      if (n > 0 && n % N == 0) { k++; peak = y; }
    }
    float dbAtEnd = 20.0f * log10f(peak);
    check(near(dbAtEnd, -60.0 * ((k - 1) * N) / (T60 * fs), 0.05), "comb: level of echo k follows T60 rule", dbAtEnd, -60.0 * ((k - 1) * N) / (T60 * fs));
  }
  }

  // Allpass: total energy of the impulse response = 1
  if (want(8)) {
  {
    Allpass<441> a; a.set(0.5f); double e = 0;
    for (int n = 0; n < 441 * 40; n++) { float y = a.process(n == 0 ? 1.0f : 0.0f); e += (double)y * y; }
    check(near(e, 1.0, 1e-3), "allpass: impulse response energy = 1", e, 1.0);
  }
  }

  // Reverb: finite, decays
  if (want(8)) {
  {
    Reverb r; double early = 0, late = 0;
    for (int n = 0; n < 5 * 44100; n++) {
      float y = r.process(n == 0 ? 1.0f : 0.0f);
      if (n < 22050) early += (double)y * y; else if (n >= 4 * 44100) late += (double)y * y;
    }
    check(late < early * 0.01 && late == late, "reverb: tail energy decays (late << early)", late, early);
  }
  }

  // ---- Stage 9: Controls -> Params ----
  if (want(9)) {
    Controls c; Params p;
    for (int i = 0; i < 6; i++) c.pot[i] = 0.0f;
    p = mapControls(c);
    check(near(p.levelDb, -40.0, 1e-3), "pots at 0: LEVEL = -40 dB", p.levelDb, -40.0);
    check(near(p.toneHz, 200.0, 1e-2), "pots at 0: TONE = 200 Hz", p.toneHz, 200.0);
    check(near(p.mix, 0.0, 1e-4), "pots at 0: MIX = 0", p.mix, 0.0);
    check(near(p.timeMs, 20.0, 1e-3), "pots at 0: TIME = 20 ms", p.timeMs, 20.0);
    check(near(p.pitchSemis, -12.0, 1e-3), "pots at 0: PITCH = -12 st", p.pitchSemis, -12.0);
    check(near(p.synthDb, -40.0, 1e-3), "pots at 0: SYNTH = -40 dB", p.synthDb, -40.0);

    for (int i = 0; i < 6; i++) c.pot[i] = 1.0f;
    p = mapControls(c);
    check(near(p.levelDb, 6.0, 1e-3), "pots at 1: LEVEL = +6 dB", p.levelDb, 6.0);
    check(near(p.toneHz, 8000.0, 1e-1), "pots at 1: TONE = 8000 Hz", p.toneHz, 8000.0);
    check(near(p.mix, 1.0, 1e-4), "pots at 1: MIX = 1", p.mix, 1.0);
    check(near(p.timeMs, 600.0, 1e-2), "pots at 1: TIME = 600 ms", p.timeMs, 600.0);
    check(near(p.pitchSemis, 12.0, 1e-3), "pots at 1: PITCH = +12 st", p.pitchSemis, 12.0);
    check(near(p.synthDb, 0.0, 1e-3), "pots at 1: SYNTH = 0 dB", p.synthDb, 0.0);

    for (int i = 0; i < 6; i++) c.pot[i] = 0.5f;
    p = mapControls(c);
    check(near(p.levelDb, -17.0, 1e-3), "pots at 0.5: LEVEL = -17 dB", p.levelDb, -17.0);
    check(near(p.toneHz, 1264.91, 1e-1), "pots at 0.5: TONE = 1264.9 Hz (geometric middle)", p.toneHz, 1264.91);
    check(near(p.mix, 0.5, 1e-4), "pots at 0.5: MIX = 0.5", p.mix, 0.5);
    check(near(p.timeMs, 109.545, 1e-2), "pots at 0.5: TIME = 109.5 ms", p.timeMs, 109.545);
    check(near(p.pitchSemis, 0.0, 1e-3), "pots at 0.5: PITCH = 0 st", p.pitchSemis, 0.0);
    check(near(p.synthDb, -20.0, 1e-3), "pots at 0.5: SYNTH = -20 dB", p.synthDb, -20.0);

    // pots are independent: moving only TONE changes only TONE
    Controls d; d.pot[1] = 1.0f;
    Params q = mapControls(d);
    check(near(q.toneHz, 8000.0, 1e-1) && near(q.levelDb, -40.0, 1e-3) && near(q.timeMs, 20.0, 1e-3),
          "only TONE moved: other params unchanged", q.toneHz, 8000.0);

    // out-of-range readings clamp to the ends
    Controls e; e.pot[0] = 1.3f; e.pot[1] = -0.2f;
    Params r = mapControls(e);
    check(near(r.levelDb, 6.0, 1e-3), "LEVEL pot 1.3 clamps to +6 dB", r.levelDb, 6.0);
    check(near(r.toneHz, 200.0, 1e-2), "TONE pot -0.2 clamps to 200 Hz", r.toneHz, 200.0);
  }

  // ---- Stage 10: Smoother ----
  if (want(10)) {
    const float tau = 0.01f;                    // 10 ms -> 441 samples at 44.1 kHz
    const int   tauN = (int)(tau * fs);
    {
      Smoother sm; sm.set(tau, fs); sm.reset(0.0f); sm.setTarget(1.0f);
      float first = sm.next();
      check(near(first, 1.0 - exp(-1.0 / 441.0), 1e-5), "first step after a 0->1 jump = k", first, 1.0 - exp(-1.0 / 441.0));
      float v = first;
      for (int n = 1; n < tauN; n++) v = sm.next();
      check(near(v, 1.0 - exp(-1.0), 5e-3), "after tau seconds: 63.2% of the way", v, 1.0 - exp(-1.0));
      for (int n = tauN; n < 5 * tauN; n++) v = sm.next();
      check(v > 0.99 && v < 1.0f, "after 5 tau: above 99% but not past the target", v, 0.993);
    }
    {
      Smoother sm; sm.set(tau, fs); sm.reset(0.0f); sm.setTarget(1.0f);
      bool mono = true, under = true; float prev = 0.0f, last = 0.0f;
      for (int n = 0; n < 3000; n++) { last = sm.next(); if (last < prev) mono = false; if (last > 1.0f) under = false; prev = last; }
      check(mono && under && last > 0.5f, "rises steadily and never overshoots", last, 0.5);
    }
    {
      Smoother sm; sm.set(tau, fs); sm.reset(0.3f);
      float a = sm.next(), b = sm.next();
      check(near(a, 0.3, 1e-6) && near(b, 0.3, 1e-6), "reset(0.3): stays at 0.3 with no new target", b, 0.3);
    }
    {
      Smoother sm; sm.set(tau, fs); sm.reset(0.0f); sm.setTarget(1.0f);
      float v = 0; for (int n = 0; n < 300; n++) v = sm.next();
      sm.setTarget(0.0f);
      float w = sm.next();
      check(w < v && near(w, v, 0.01) && v > 0.3f, "new target mid-glide: no jump, turns around", w, v);
    }
  }

  // ---- Stage 11: EffectSelect ----
  if (want(11)) {
    {
      EffectSelect es; Controls c;
      es.update(c);
      check(es.currentFx() == Fx::Dly && !es.fxOn(), "power-up: DLY selected, effect off", (double)(int)es.currentFx(), 0);
    }
    {
      EffectSelect es; Controls c;
      es.update(c);
      c.btn[1] = true;  es.update(c);
      check(es.currentFx() == Fx::Cho, "press BTN2 -> CHO", (double)(int)es.currentFx(), 1);
      for (int i = 0; i < 10; i++) es.update(c);
      c.btn[1] = false; es.update(c);
      check(es.currentFx() == Fx::Cho, "held then released: still CHO", (double)(int)es.currentFx(), 1);
    }
    {
      EffectSelect es; Controls c;
      es.update(c);
      c.btn[0] = true; es.update(c);               // hold BTN1
      c.btn[1] = true; es.update(c);               // then press BTN2 as well
      check(es.currentFx() == Fx::Cho, "BTN1 held, BTN2 pressed: newest press wins", (double)(int)es.currentFx(), 1);
      c.btn[1] = false; es.update(c);              // release BTN2, BTN1 still down
      check(es.currentFx() == Fx::Cho, "releasing BTN2 does not re-select held BTN1", (double)(int)es.currentFx(), 1);
    }
    {
      EffectSelect es; Controls c;
      es.update(c);
      c.btn[0] = true; c.btn[2] = true; es.update(c);   // two edges on the same tick
      check(es.currentFx() == Fx::Dst, "BTN1+BTN3 same tick: higher number (DST) wins", (double)(int)es.currentFx(), 2);
    }
    {
      EffectSelect es; Controls c;
      es.update(c);
      for (int i = 4; i < 8; i++) { c.btn[i] = true; es.update(c); c.btn[i] = false; es.update(c); }
      check(es.currentFx() == Fx::Dly && !es.fxOn(), "BTN5-8 (notes) do not change effect or bypass", (double)(int)es.currentFx(), 0);
    }
    {
      EffectSelect es; Controls c;
      es.update(c);
      c.fsw = true;  es.update(c);
      check(es.fxOn(), "footswitch press: effect on", es.fxOn(), 1);
      for (int i = 0; i < 20; i++) es.update(c);
      check(es.fxOn(), "footswitch held 20 ticks: still on (no re-toggle)", es.fxOn(), 1);
      c.fsw = false; es.update(c);
      check(es.fxOn(), "footswitch released: still on", es.fxOn(), 1);
      c.fsw = true;  es.update(c);
      check(!es.fxOn(), "second press: effect off", es.fxOn(), 0);
    }
  }

  // ---- Stage 12: Voice ----
  if (want(12)) {
    const int N1 = 44100;
    auto crossings = [](Voice &v, int n, float bend) {
      int c = 0; float prev = 0.0f;
      for (int i = 0; i < n; i++) { float y = v.next(bend); if (prev < 0.0f && y >= 0.0f) c++; prev = y; }
      return c;
    };
    {
      Voice v; v.set(0.001f, 0.001f, 1.0f, 0.01f, fs);
      float peak = 0; for (int i = 0; i < 100; i++) peak = fmaxf(peak, fabsf(v.next(0.0f)));
      check(peak == 0.0f && !v.active(), "silent and inactive before any note", peak, 0.0);
      v.noteOn(69);
      check(v.active(), "active after noteOn", v.active(), 1);
    }
    {
      Voice v; v.set(0.001f, 0.001f, 1.0f, 0.01f, fs); v.noteOn(69);
      int c = crossings(v, N1, 0.0f);
      check(near(c, 440, 2), "noteOn(69): 440 Hz (crossings in 1 s)", c, 440);
    }
    {
      Voice v; v.set(0.001f, 0.001f, 1.0f, 0.01f, fs); v.noteOn(69);
      int c = crossings(v, N1, 12.0f);
      check(near(c, 880, 3), "bend +12 st: 880 Hz", c, 880);
    }
    {
      Voice v; v.set(0.001f, 0.001f, 1.0f, 0.01f, fs); v.noteOn(69);
      crossings(v, N1 / 2, 0.0f);                       // half a second at A4
      int c = crossings(v, N1 / 2, 12.0f);              // pot turned while holding: 880 Hz for 0.5 s
      check(near(c, 440, 3), "bend changed while held: pitch follows", c, 440);
    }
    {
      Voice v; v.set(0.001f, 0.001f, 1.0f, 0.01f, fs); v.noteOn(69);
      crossings(v, 1000, 0.0f);
      v.noteOn(81);                                     // new press while sounding: last press wins
      crossings(v, 2000, 0.0f);                         // let the retrigger settle
      int c = crossings(v, N1, 0.0f);
      check(near(c, 880, 3), "noteOn(81) while sounding: retunes to 880 Hz", c, 880);
    }
    {
      Voice v; v.set(0.001f, 0.001f, 0.5f, 0.01f, fs); v.noteOn(69);
      for (int i = 0; i < 2000; i++) v.next(0.0f);      // past attack + decay
      float peak = 0; for (int i = 0; i < 1000; i++) peak = fmaxf(peak, fabsf(v.next(0.0f)));
      check(near(peak, 0.5, 0.01), "sustain level 0.5 scales the amplitude", peak, 0.5);
    }
    {
      Voice v; v.set(0.001f, 0.001f, 1.0f, 0.01f, fs); v.noteOn(69);
      for (int i = 0; i < 2000; i++) v.next(0.0f);
      v.noteOff();
      for (int i = 0; i < 1000; i++) v.next(0.0f);      // release is 441 samples
      float peak = 0; for (int i = 0; i < 100; i++) peak = fmaxf(peak, fabsf(v.next(0.0f)));
      check(peak < 1e-6f && !v.active(), "after release: silent and inactive", peak, 0.0);
    }
  }

  // ---- Stage 13: Engine ----
  if (want(13)) {
    static Engine E;
    auto mk = [](float levelDb, float toneU, float mixU, float timeU, float synthDb) {
      Controls c;
      c.pot[0] = (levelDb + 40.0f) / 46.0f; c.pot[1] = toneU; c.pot[2] = mixU;
      c.pot[3] = timeU; c.pot[4] = 0.5f; c.pot[5] = (synthDb + 40.0f) / 40.0f;
      return c;
    };
    auto sine = [&](int n) { return 0.5f * sinf(2.0f * kPi * 440.0f * n / fs); };
    auto toggleFx = [&](Controls c) { c.fsw = true; E.setControls(c); c.fsw = false; E.setControls(c); };
    auto pickFx   = [&](Controls c, int i) { c.btn[i] = true; E.setControls(c); c.btn[i] = false; E.setControls(c); };

    // ---- 13a: bypass, LEVEL, smoothing ----
    {
      E.begin(fs); Controls c = mk(0.0f, 0.0f, 1.0f, 0.5f, -40.0f); E.setControls(c);
      float err = 0; for (int n = 0; n < 2000; n++) { float x = sine(n); err = fmaxf(err, fabsf(E.process(x) - x)); }
      check(err < 1e-4f, "13a FX off, 0 dB: out = in (TONE and MIX ignored)", err, 0.0);
    }
    {
      E.begin(fs); Controls c = mk(-6.0206f, 1.0f, 0.0f, 0.5f, -40.0f); E.setControls(c);
      float err = 0; for (int n = 0; n < 2000; n++) { float x = sine(n); err = fmaxf(err, fabsf(E.process(x) - 0.5f * x)); }
      check(err < 2e-3f, "13a LEVEL -6.02 dB halves the signal", err, 0.0);
    }
    {
      E.begin(fs); Controls c = mk(0.0f, 1.0f, 0.0f, 0.5f, -40.0f); E.setControls(c);
      float y = 0; for (int n = 0; n < 100; n++) y = E.process(1.0f);
      check(near(y, 1.0, 1e-3), "13a steady input 1.0 at 0 dB gives 1.0", y, 1.0);
      c = mk(-40.0f, 1.0f, 0.0f, 0.5f, -40.0f); E.setControls(c);
      float first1 = E.process(1.0f);
      check(first1 > 0.99f && first1 < 1.0f, "13a LEVEL pot step: first sample has not jumped", first1, 0.9977);
      for (int n = 0; n < 8000; n++) y = E.process(1.0f);
      check(near(y, 0.01, 2e-3), "13a LEVEL pot step: settles at -40 dB", y, 0.01);
    }
    // ---- 13b: effect chain ----
    {
      E.begin(fs); Controls c = mk(0.0f, 1.0f, 0.0f, 0.5f, -40.0f); E.setControls(c); toggleFx(c);
      double si = 0, so = 0;
      for (int n = 0; n < 8000; n++) { float x = sine(n), y = E.process(x); if (n >= 4000) { si += x * x; so += y * y; } }
      double ratio = sqrt(so / si);
      check(near(ratio, 1.0, 0.01), "13b FX on, MIX 0: dry passes through TONE (RMS ratio)", ratio, 1.0);
    }
    {
      E.begin(fs); Controls c = mk(0.0f, 1.0f, 1.0f, 0.5f, -40.0f); E.setControls(c); toggleFx(c);
      float y0 = E.process(1.0f), peak = 0; int at = 0;
      for (int n = 1; n < 10000; n++) { float y = fabsf(E.process(0.0f)); if (y > peak) { peak = y; at = n; } }
      check(fabsf(y0) < 0.01f, "13b DLY, MIX 1: dry impulse is gone", fabsf(y0), 0.0);
      check(abs(at - 4831) <= 3, "13b DLY: first echo at TIME = 109.5 ms (sample index)", at, 4831);
    }
    {
      E.begin(fs); Controls c = mk(0.0f, 1.0f, 1.0f, 1.0f, -40.0f); E.setControls(c); toggleFx(c); pickFx(c, 2);
      float peak = 0; for (int n = 0; n < 8000; n++) { float y = E.process(sine(n)); if (n >= 4000) peak = fmaxf(peak, fabsf(y)); }
      check(peak > 0.9f && peak < 1.2f, "13b DST, MIX 1, TIME max: 0.5 input driven near full scale", peak, 1.0);
    }
    {
      E.begin(fs); Controls c = mk(0.0f, 1.0f, 1.0f, 0.5f, -40.0f); E.setControls(c); toggleFx(c); pickFx(c, 1);
      float y0 = E.process(1.0f), peak = 0; int at = 0;
      for (int n = 1; n < 3000; n++) { float y = fabsf(E.process(0.0f)); if (y > peak) { peak = y; at = n; } }
      check(fabsf(y0) < 0.01f, "13b CHO, MIX 1: dry impulse is gone", fabsf(y0), 0.0);
      check(at > 500 && at < 820 && peak > 0.05f, "13b CHO: impulse comes back 15 ms +/- 3 ms later", at, 661);
    }
    {
      E.begin(fs); Controls c = mk(0.0f, 1.0f, 1.0f, 0.5f, -40.0f); E.setControls(c); toggleFx(c); pickFx(c, 3);
      float y0 = E.process(1.0f); double early = 0, mid = 0, late = 0;
      for (int n = 1; n < 5 * 44100; n++) {
        float y = E.process(0.0f);
        if (n >= 2000 && n < 22050) mid += (double)y * y;
        if (n < 22050) early += (double)y * y;
        if (n >= 4 * 44100) late += (double)y * y;
      }
      check(fabsf(y0) < 0.01f, "13b REV, MIX 1: dry impulse is gone", fabsf(y0), 0.0);
      check(mid > 1e-4, "13b REV: a reverb tail appears", mid, 1e-3);
      check(late < early * 1e-3 && late == late, "13b REV: tail dies away", late, early * 1e-3);
    }
    // ---- 13c: synth voice ----
    {
      E.begin(fs); Controls c = mk(0.0f, 1.0f, 0.0f, 0.5f, 0.0f); E.setControls(c);
      c.btn[4] = true; E.setControls(c);
      float peak = 0; for (int n = 0; n < 13230; n++) { float y = E.process(0.0f); if (n >= 11230) peak = fmaxf(peak, fabsf(y)); }
      check(peak > 0.66f && peak < 0.72f, "13c BTN5 plays a note at the sustain level 0.7", peak, 0.7);
      c.btn[4] = false; E.setControls(c);
      peak = 0; for (int n = 0; n < 22050; n++) { float y = E.process(0.0f); if (n >= 21050) peak = fmaxf(peak, fabsf(y)); }
      check(peak < 1e-4f, "13c releasing the key fades to silence", peak, 0.0);
    }
    {
      E.begin(fs); Controls c = mk(0.0f, 1.0f, 0.0f, 0.5f, -6.0206f); E.setControls(c);
      c.btn[4] = true; E.setControls(c);
      float peak = 0; for (int n = 0; n < 13230; n++) { float y = E.process(0.0f); if (n >= 11230) peak = fmaxf(peak, fabsf(y)); }
      check(peak > 0.33f && peak < 0.37f, "13c SYNTH -6 dB halves the note", peak, 0.35);
    }
    {
      E.begin(fs); Controls c = mk(0.0f, 1.0f, 0.0f, 0.5f, 0.0f); E.setControls(c);
      c.btn[4] = true; E.setControls(c); for (int n = 0; n < 2000; n++) E.process(0.0f);
      c.btn[5] = true; E.setControls(c); for (int n = 0; n < 2000; n++) E.process(0.0f);
      c.btn[4] = false; E.setControls(c);                       // older key released, newer still held
      float peak = 0; for (int n = 0; n < 13230; n++) { float y = E.process(0.0f); if (n >= 11230) peak = fmaxf(peak, fabsf(y)); }
      check(peak > 0.6f && peak < 0.72f, "13c releasing the older key keeps the newer note", peak, 0.7);
      c.btn[5] = false; E.setControls(c);
      peak = 0; for (int n = 0; n < 22050; n++) { float y = E.process(0.0f); if (n >= 21050) peak = fmaxf(peak, fabsf(y)); }
      check(peak < 1e-4f, "13c releasing the newer key stops the sound", peak, 0.0);
    }
  }

  // ---- AudioIO + FakeAudioIO (run with: make stage N=14) ----
  if (want(14)) {
    {
      FakeAudioIO fake; AudioIO &io = fake; CbCount k;
      io.setCallback(countCb, &k); fake.setSine(440.0f, 0.5f, fs);
      bool ok = io.begin(); fake.run(10);
      check(ok, "begin() returns true", ok, 1);
      check(k.calls == 10 && k.all128, "run(10): 10 callbacks of exactly 128 samples", k.calls, 10);
      check(fake.inputs().size() == 1280 && fake.outputs().size() == 1280, "both recordings hold 1280 samples", (double)fake.outputs().size(), 1280);
      const std::vector<float> &in = fake.inputs();
      float step = 0; for (size_t n = 1; n < in.size(); n++) step = fmaxf(step, fabsf(in[n] - in[n - 1]));
      check(in.size() == 1280 && step < 0.04f, "sine continues across block boundaries (no jump)", step, 0.03);
      check(in.size() > 25 && in[0] == 0.0f && near(in[25], 0.5 * sin(2.0 * 3.14159265358979 * 440.0 * 25 / fs), 1e-4),
            "input is 0.5 * sin(2 pi 440 n / fs)", in.size() > 25 ? in[25] : 0.0, 0.5 * sin(2.0 * 3.14159265358979 * 440.0 * 25 / fs));
      io.begin(); fake.run(1);
      check(fake.outputs().size() == 128 && fake.inputs().size() == 128 && fake.inputs()[0] == 0.0f,
            "begin() again: recordings cleared, sine restarts at phase 0", (double)fake.outputs().size(), 128);
    }
    {
      static Engine E; E.begin(fs);
      Controls c; c.pot[0] = 40.0f / 46.0f; c.pot[5] = 0.0f; E.setControls(c);       // FX off, LEVEL 0 dB
      FakeAudioIO fake; fake.setSine(440.0f, 0.5f, fs); fake.setCallback(engineCb, &E); fake.begin(); fake.run(20);
      float err = 0; for (size_t n = 0; n < fake.inputs().size(); n++) err = fmaxf(err, fabsf(fake.outputs()[n] - fake.inputs()[n]));
      check(fake.inputs().size() == 2560 && err < 1e-4f, "engine in bypass through the fake: output = input", err, 0.0);
    }
    {
      static Engine E; E.begin(fs);
      Controls c; c.pot[0] = (-6.0206f + 40.0f) / 46.0f; c.pot[5] = 0.0f; E.setControls(c);
      FakeAudioIO fake; fake.setSine(440.0f, 0.5f, fs); fake.setCallback(engineCb, &E); fake.begin(); fake.run(20);
      float err = 0; for (size_t n = 0; n < fake.inputs().size(); n++) err = fmaxf(err, fabsf(fake.outputs()[n] - 0.5f * fake.inputs()[n]));
      check(fake.inputs().size() == 2560 && err < 2e-3f, "engine LEVEL -6 dB through the fake: output = input / 2", err, 0.0);
    }
  }

  printf("\n%s (%d failed)\n", fails ? "SOME TESTS FAILED" : "ALL TESTS PASSED", fails);
  return fails ? 1 : 0;
}
