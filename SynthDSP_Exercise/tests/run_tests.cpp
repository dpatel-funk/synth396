// Native test program: checks every DSP block against its theory.
// Build and run on a computer:   make test      (or ./run_tests.sh)
#include <stdio.h>
#include <stdlib.h>
#include "../DspLab/src/dsp/All.h"
using namespace dsp;

static int fails = 0;
static int stage = 0;                       // 0 = run everything
static bool want(int k) { return stage == 0 || stage == k; }
static void check(bool ok, const char *name, double got, double want) {
  printf("%s  %-44s got %-12.5g want %-12.5g\n", ok ? "PASS" : "FAIL", name, got, want);
  if (!ok) fails++;
}
static bool near(double a, double b, double tol) { return fabs(a - b) <= tol; }

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

  printf("\n%s (%d failed)\n", fails ? "SOME TESTS FAILED" : "ALL TESTS PASSED", fails);
  return fails ? 1 : 0;
}
