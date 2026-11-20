// Compile-checks DSP_funcs.ino against a mock Arduino core and prints every plot test.
#include "arduino_mock.h"
#include "../DSP_funcs/DSP_funcs.ino"
int main(int argc, char **argv) {
  setup();
  int t = argc > 1 ? argv[1][0] - 48 : 1; runTest(t); if (t == 7) runTest(7);
  return 0;
}
