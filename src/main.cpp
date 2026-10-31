#include "collision.h"
#include "color_sensor.h"
#include "fsm.h"
#include "motors.h"
#include "websocket.h"

FSM fsm;

void setup() {
  Serial.begin(115200);
  setup_motors();
  websocketInit();
  colorInit();
  setupCollisionDetection();
  delay(3000);
}

void loop() { fsm.tick(); }
