// #include "collision.h"
// #include "color_sensor.h"
// #include "fsm.h"
// #include "motors.h"
// #include "websocket.h"
//
// FSM fsm;
//
// void setup() {
//   Serial.begin(115200);
//   setupColorSensors();
//   // setup_motors();
//   //  websocketInit();
//   // setupCollisionDetection();
// }
//
// void loop() {
//   Serial.print("LEFT   ");
//   leftColorSensor.testReading();
//   Serial.print("RIGHT  ");
//   rightColorSensor.testReading();
//   Serial.println();
//   delay(200);
//   // fsm.tick();
//   // delay(10);
// }
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
// Detection test: one sensor at a time.
// Prints the flashed R/G/B reading and the classified color.
// Send 'l' or 'R' over serial to switch sensors.

#include "color_sensor.h"
#include "drive.h"
#include "motors.h"
#include <Arduino.h>

constexpr unsigned long PRINT_PERIOD_MS = 300;

bool testLeft = true;

void setup() {
  Serial.begin(115200);
  delay(1000);

  setup_motors();
}

void loop() {
  drive_forward(300);
  delay(PRINT_PERIOD_MS);
}
