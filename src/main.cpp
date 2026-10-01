#include <Arduino.h>
#include "collision_detection.h"

void setup() {
    Serial.begin(115200);

    setupCollisionDetection();

    delay(1000);
}

void loop() {
    CollisionReading reading = readCollisionDetection();

    Serial.printf(
        "background: %4d  illuminated: %4d  reflected: %4d  %s\n",
        reading.background,
        reading.illuminated,
        reading.reflected,
        reading.imminent_collision ? "ABOUT TO COLLIDE" : "SAFE"
    );
    delay(50);
}
