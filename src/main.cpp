#include "websocket.h"
#include "motors.h"
#include "fsm.h"

Command command;
FSM fsm;

void setup() {
    Serial.begin(115200);
    setup_motors();

    websocketInit();
}

void loop() {
    fsm.tick();
}

