#include "websocket.h"
#include "collision.h"
#include "motors.h"
#include "fsm.h"
#include "color_sensor.h"

FSM fsm;

void setup() {
    Serial.begin(115200);
    setup_motors();
    websocketInit();
    colorInit();
    setupCollisionDetection();
}

void loop() {
    fsm.tick();
}