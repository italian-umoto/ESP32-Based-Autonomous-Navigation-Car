#include "collision.h"
#include "pins.h"
#include <Arduino.h>

<<<<<<< HEAD
static const int COLLISION_THRESHOLD = 50;
static const int NUM_SAMPLES = 4;
=======
static const int COLLISION_THRESHOLD = 400;
static const int NUM_SAMPLES = 10;

long filtered_dist;
>>>>>>> fe5872ea04b361cd6b877682ef066f4fb9d73b47

static int readAverageCollison() {
    long dist = analogRead(COLLISION_SENSOR_PIN)
    filtered_dist = (filtered_dist * (NUM_SAMPLES - 1) + dist) / NUM_SAMPLES;

    return filtered_dist;
}

void setupCollisionDetection() {
    pinMode(IR_LED_PIN, OUTPUT);
    digitalWrite(IR_LED_PIN, LOW);

    analogReadResolution(12);
    analogSetPinAttenuation(COLLISION_SENSOR_PIN, ADC_11db);
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
    // return {background, illuminated, reflected, reflected > COLLISION_THRESHOLD};
    return {background, illuminated, reflected, false};

}