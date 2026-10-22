#include "color_sensor.h"
#include "websocket.h"
#include "collision.h"
#include "motors.h"
#include "battery.h"
#include "fsm.h"

FSM fsm;

void setup() {
    Serial.begin(115200);
    setup_motors();
    websocketInit();
    colorInit();
    batteryInit();
    setupCollisionDetection();
}

void loop() {
    fsm.tick();
}

