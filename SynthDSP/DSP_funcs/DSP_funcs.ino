// DSP_funcs - check your DSP formulas visually with the Arduino IDE's Serial Plotter.
//
// How to use (Arduino IDE 2.x, board: Teensy 4.1 via Teensyduino):
//   1. Upload this sketch.
//   2. Open  Tools > Serial Plotter  (baud rate does not matter on Teensy USB).
//   3. Change TEST_TO_RUN below and re-upload, OR open Serial Monitor and send a
//      digit 1-6 (the Plotter has no send box), then switch back to the Plotter.
//
//   1 = low-pass filter frequency response  (measured vs theory, in dB)
//   2 = echo impulse response               (repeats at D, 2D, 3D, shrinking by f)
//   3 = soft-clip transfer curves           (output vs input for 3 drive values)
//   4 = oscillator waveforms                (sine, saw, square, triangle)
//   5 = ADSR envelope                       (note on at 0 s, note off at 0.8 s)
//   6 = reverb impulse response             (decaying noise-like tail)
//   7 = LIVE controls (breadboard): pots and buttons -> parameter values, as text.
//       Open Serial MONITOR (not the Plotter) for this one. Needs stage 1 done.
//
// No Teensy Audio library is used: every DSP block comes from src/dsp/*.h.

#include "src/dsp/All.h"
using namespace dsp;

const int TEST_TO_RUN = 1;
const float FS = 44100.0f;

static void plot2(const char *a, float x, const char *b, float y) {
  Serial.print(a); Serial.print(':'); Serial.print(x); Serial.print(',');
  Serial.print(b); Serial.print(':'); Serial.println(y);
}

static void testFilter() {
  for (int i = 0; i < 64; i++) {
    const float f = 50.0f * powf(400.0f, i / 63.0f);  // 50 Hz .. 20 kHz, log spaced
    BiquadLowpass lp; lp.set(1000.0f, 0.70710678f, FS);
    float meas = measureGainDb([&](float x) { return lp.process(x); }, f, FS);
    float theo = gainToDb(butterworthMagnitude(f, 1000.0f, FS));
    plot2("measured_dB", fmaxf(meas, -60.0f), "theory_dB", fmaxf(theo, -60.0f));
  }
}

static void testEcho() {
  static Echo<2048> e;
  e.clear(); e.set(100, 0.5f, 1.0f);
  for (int n = 0; n < 600; n++) {
    const float x = (n == 0) ? 1.0f : 0.0f;
    plot2("in", x, "out", e.process(x));
  }
}

static void testSoftClip() {
  const float drives[3] = {1.0f, 5.0f, 20.0f};
  for (int i = -50; i <= 50; i++) {
    const float x = i / 50.0f;
    Serial.print("drive1:");  Serial.print(softClip(x, drives[0])); Serial.print(',');
    Serial.print("drive5:");  Serial.print(softClip(x, drives[1])); Serial.print(',');
    Serial.print("drive20:"); Serial.println(softClip(x, drives[2]));
  }
}

static void testOsc() {
  Osc o1, o2, o3, o4;
  const float f = FS / 100.0f;  // 100 samples per cycle -> 2 cycles over 200 points
  o1.setFreq(f, FS); o2.setFreq(f, FS); o3.setFreq(f, FS); o4.setFreq(f, FS);
  for (int n = 0; n < 200; n++) {
    Serial.print("sine:");     Serial.print(o1.next(Wave::Sine));     Serial.print(',');
    Serial.print("saw:");      Serial.print(o2.next(Wave::Saw));      Serial.print(',');
    Serial.print("square:");   Serial.print(o3.next(Wave::Square));   Serial.print(',');
    Serial.print("triangle:"); Serial.println(o4.next(Wave::Triangle));
  }
}

static void testAdsr() {
  const float fsEnv = 1000.0f;  // 1 point per ms so a 1.5 s envelope fits the plot
  Adsr a; a.set(0.1f, 0.2f, 0.6f, 0.4f, fsEnv);
  a.noteOn();
  for (int n = 0; n < 1500; n++) {
    if (n == 800) a.noteOff();
    Serial.print("envelope:"); Serial.println(a.next());
  }
}

static void testReverb() {
  static Reverb r;
  r.clear();
  for (int n = 0; n < 20000; n++) {
    const float y = r.process(n == 0 ? 1.0f : 0.0f);
    if (n % 20 == 0) Serial.print("reverb:"), Serial.println(y);  // every 20th sample
  }
}


// ---- LIVE test (breadboard) ------------------------------------------------------
// Pin map copied from the earlier controls sketch; check it against your wiring.
// Pots: wiper to the pin, ends to 3.3 V and GND.  Buttons: one side to the pin, other to GND
// (INPUT_PULLUP, so pressed reads LOW).  Footswitch the same way.
const int POT_PINS[6] = {24, 25, 26, 27, 40, 41};   // LEVEL TONE MIX TIME PITCH SYNTH
const int BTN_PINS[8] = {28, 29, 30, 31, 32, 33, 34, 35};
const int FSW_PIN = 37, LED_PIN = 36;
const float NOTES[4] = {60, 62, 64, 67};            // BTN5-8 = C D E G

static float potU[6];          // smoothed 0..1
static bool liveInit = false;
static bool fxOn = false, fswPrev = false;

static void testLive() {
  if (!liveInit) {
    analogReadResolution(10);
    for (int i = 0; i < 8; i++) pinMode(BTN_PINS[i], INPUT_PULLUP);
    pinMode(FSW_PIN, INPUT_PULLUP);
    pinMode(LED_PIN, OUTPUT);
    for (int i = 0; i < 6; i++) potU[i] = analogRead(POT_PINS[i]) / 1023.0f;
    liveInit = true;
  }
  for (int i = 0; i < 6; i++)                        // one-pole smoothing of a noisy reading
    potU[i] += 0.2f * (analogRead(POT_PINS[i]) / 1023.0f - potU[i]);

  const bool fsw = digitalRead(FSW_PIN) == LOW;      // footswitch toggles on the press edge
  if (fsw && !fswPrev) fxOn = !fxOn;
  fswPrev = fsw;
  digitalWrite(LED_PIN, fxOn ? HIGH : LOW);

  // the mapping from your control map, built from your stage-1 functions
  const float levelDb = linMap(potU[0], -40.0f, 6.0f);
  const float toneHz  = expMap(potU[1], 200.0f, 8000.0f);
  const float mix     = linMap(potU[2], 0.0f, 1.0f);
  const float timeMs  = expMap(potU[3], 20.0f, 600.0f);
  const float bend    = linMap(potU[4], -12.0f, 12.0f);
  const float synthDb = linMap(potU[5], -40.0f, 0.0f);

  Serial.print("LEVEL "); Serial.print(levelDb); Serial.print(" dB (x"); Serial.print(dbToGain(levelDb));
  Serial.print(") | TONE "); Serial.print(toneHz);
  Serial.print(" Hz | MIX "); Serial.print(mix);
  Serial.print(" | TIME "); Serial.print(timeMs);
  Serial.print(" ms | PITCH "); Serial.print(bend);
  Serial.print(" st | SYNTH "); Serial.print(synthDb);
  Serial.print(" dB | FX "); Serial.print(fxOn ? "ON" : "off");
  Serial.print(" | BTN1-4 ");
  for (int i = 0; i < 4; i++) Serial.print(digitalRead(BTN_PINS[i]) == LOW ? '#' : '.');
  Serial.print(" | notes:");
  for (int i = 0; i < 4; i++)
    if (digitalRead(BTN_PINS[4 + i]) == LOW) { Serial.print(' '); Serial.print(midiToHz(NOTES[i], bend)); Serial.print("Hz"); }
  Serial.println();
}

static void runTest(int t) {
  switch (t) {
    case 1: testFilter(); break;
    case 2: testEcho(); break;
    case 3: testSoftClip(); break;
    case 4: testOsc(); break;
    case 5: testAdsr(); break;
    case 6: testReverb(); break;
    case 7: testLive(); break;
    default: break;
  }
}

static int current = TEST_TO_RUN;

void setup() {
  Serial.begin(115200);
  while (!Serial && millis() < 3000) {}
}

void loop() {
  while (Serial.available()) {
    const int c = Serial.read();
    if (c >= '1' && c <= '7') current = c - '0';
  }
  runTest(current);
  delay(current == 7 ? 100 : 1500);  // live view updates 10x/s; plots pause so each run is visible
}
