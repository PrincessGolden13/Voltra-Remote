#include <Arduino.h>
#include "hardware/boards/prototype_21/Board.h"
#include "ble/NimBleTransport.h"
// Unreachable while target validation blocks compilation. No radio or GPIO setup.
void setup() {}
void loop() { delay(100); }
