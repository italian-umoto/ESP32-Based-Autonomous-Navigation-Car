#include "color_sensor.h"
#include "websocket.h"
#include "collision.h"
#include "motors.h"
#include "fsm.h"

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

    const colorReading reading = single_read();
    const DetectedColor color = classifyColor(reading);

    printReading(reading, color);
}

