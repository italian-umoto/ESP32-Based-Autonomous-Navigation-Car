#include "collision.h"
#include "color_sensor.h"
#include "fsm.h"
#include "motors.h"
#include "websocket.h"

FSM fsm;

void setup() {
  Serial.begin(115200);
  // setupColorSensors();
  setup_motors();
  websocketInit();
  // setupCollisionDetection();
}

void loop() {
  // Serial.print("LEFT   ");
  // leftColorSensor.testReading();
  // Serial.print("RIGHT  ");
  // rightColorSensor.testReading();
  // Serial.println();
  // delay(200);
  fsm.tick();
  delay(10);
}
