#include "collision.h"
#include <Arduino.h>

static const int IR_LED_PIN = 5;
static const int SENSOR_PIN = 4;
static const int COLLISION_THRESHOLD = 900;
static const int NUM_SAMPLES = 4;

static int readAverageCollison() {
    long sum = 0;

    for (int i = 0; i < NUM_SAMPLES; i++) {
        sum += analogRead(SENSOR_PIN);
    }

    return sum / NUM_SAMPLES;
}

void setupCollisionDetection() {
    pinMode(IR_LED_PIN, OUTPUT);
    digitalWrite(IR_LED_PIN, LOW);

    analogReadResolution(12);
    analogSetPinAttenuation(SENSOR_PIN, ADC_11db);
}

CollisionReading readCollisionDetection() {
    digitalWrite(IR_LED_PIN, LOW);
    delayMicroseconds(300);
    int background = readAverageCollison();

    digitalWrite(IR_LED_PIN, HIGH);
    delayMicroseconds(300);
    int illuminated = readAverageCollison();

    digitalWrite(IR_LED_PIN, LOW);

    int reflected = illuminated - background;
    return {background, illuminated, reflected, reflected > COLLISION_THRESHOLD};
}


