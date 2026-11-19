#include "collision.h"
#include "color_sensor.h"
#include "fsm.h"
#include "motors.h"
#include "websocket.h"

FSM fsm;

void setup() {

  delay(2500);

  Serial.begin(115200);

  setup_motors();
  colorInit();
  setupCollisionDetection();

  websocketInit();

  delay(500);
}

void loop() {
  fsm.tick();
}

// void setup() {
//   Serial.begin(115200);
//   colorInit();
// }
//
// void loop() {
//   fsm.readSensors();
//   delay(500);
// }
