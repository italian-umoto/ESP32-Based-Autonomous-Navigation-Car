#include "color_sensor.h"
#include "websocket.h"
#include "collision.h"
#include "motors.h"
#include "fsm.h"

FSM fsm;

void setup() {
    Serial.begin(115200);
    setupColorSensors();
    setup_motors();
    websocketInit();
    setupCollisionDetection();
}

void loop() {
    fsm.tick();
}

