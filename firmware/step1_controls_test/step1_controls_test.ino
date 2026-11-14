// STEP 1 - controls test (no audio yet).
// Same pin numbers as the PCB, so this code will run unchanged on the real board.
// Prints pot values (0.0-1.0) and button events to the Serial Monitor.
#include <Bounce2.h>   // ships with Teensyduino

const uint8_t POT_PINS[6] = {24, 25, 26, 27, 40, 41};          // POT1..POT6 (RV1..RV6)
const uint8_t BTN_PINS[8] = {28, 29, 30, 31, 32, 33, 34, 35};  // BTN1..BTN8 (SW1..SW8)
const uint8_t FSW_PIN = 37;   // footswitch to GND
const uint8_t LED_PIN = 36;   // LED through 330 ohm to GND: HIGH = on

Bounce btn[8];
Bounce fsw;

float potSmooth[6];       // smoothed reading, 0..1023
int   potReported[6];     // last value we printed
bool  effectOn = false;

void setup() {
  Serial.begin(115200);
  analogReadResolution(10);     // 0..1023
  analogReadAveraging(8);       // hardware averaging removes most noise

  for (int i = 0; i < 8; i++) {
    btn[i].attach(BTN_PINS[i], INPUT_PULLUP);  // button connects pin to GND when pressed
    btn[i].interval(5);                        // 5 ms debounce
  }
  fsw.attach(FSW_PIN, INPUT_PULLUP);
  fsw.interval(10);
  pinMode(LED_PIN, OUTPUT);

  for (int i = 0; i < 6; i++) {
    potSmooth[i] = analogRead(POT_PINS[i]);
    potReported[i] = (int)potSmooth[i];
  }
  Serial.println("Controls test ready");
}

void loop() {
  // Pots: low-pass filter, then only report when the value moves by 3+ counts
  for (int i = 0; i < 6; i++) {
    int raw = analogRead(POT_PINS[i]);
    potSmooth[i] += 0.2f * (raw - potSmooth[i]);
    int v = (int)(potSmooth[i] + 0.5f);
    if (abs(v - potReported[i]) >= 3) {
      potReported[i] = v;
      Serial.printf("POT%d = %.3f\n", i + 1, v / 1023.0f);
    }
  }

  // Buttons: print press and release
  for (int i = 0; i < 8; i++) {
    btn[i].update();
    if (btn[i].fell()) Serial.printf("BTN%d pressed\n", i + 1);
    if (btn[i].rose()) Serial.printf("BTN%d released\n", i + 1);
  }

  // Footswitch toggles the LED
  fsw.update();
  if (fsw.fell()) {
    effectOn = !effectOn;
    digitalWrite(LED_PIN, effectOn);
    Serial.printf("Footswitch -> effect %s\n", effectOn ? "ON" : "OFF");
  }
}
