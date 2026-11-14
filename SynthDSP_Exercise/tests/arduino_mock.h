// Tiny stand-in for the Arduino core so the sketch can be compile-checked on a computer.
#pragma once
#include <stdio.h>
struct MockSerial {
  void begin(int) {}
  explicit operator bool() const { return true; }
  int available() { return 0; }
  int read() { return -1; }
  void print(const char *s) { fputs(s, stdout); }
  void print(char c) { fputc(c, stdout); }
  void print(float v) { printf("%g", v); }
  void println() { puts(""); }
  void println(const char *s) { puts(s); }
  void println(float v) { printf("%g\n", v); }
};
static MockSerial Serial;
inline unsigned long millis() { return 0; }
#define INPUT_PULLUP 2
#define OUTPUT 1
#define HIGH 1
#define LOW 0
inline void pinMode(int, int) {}
inline void analogReadResolution(int) {}
inline int analogRead(int pin) { return 100 * (pin % 10); }   // fake pot values
inline int digitalRead(int pin) { return pin == 28 || pin == 33 ? 0 : 1; }  // BTN1 and BTN6 pressed
inline void digitalWrite(int, int) {}
inline void delay(unsigned long) {}
void setup(); void loop();
