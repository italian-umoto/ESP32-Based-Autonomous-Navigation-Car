#include "fsm.h"
//#include "websocket.h"

FSM fsm;

void setup() {
    Serial.begin(115200);
    //websocket_init();
}

void loop() {
    fsm.tick();
    delay(10);   // or whatever cadence makes sense
}
