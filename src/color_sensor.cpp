#include "color_sensor.h"
#include "pins.h"
#include <Arduino.h>

constexpr int NUM_SAMPLES = 16;
constexpr int DARK_THRESHOLD = 5;
constexpr int WHITE_THRESHOLD = 150;
constexpr float COLOR_DOMINANCE = 1.1f;

constexpr int LED_SETTLE_MS = 10;
constexpr int BETWEEN_READINGS_MS = 10;

void colorInit() {
    pinMode(RED_LED_PIN, OUTPUT);
    pinMode(GREEN_LED_PIN, OUTPUT);
    pinMode(BLUE_LED_PIN, OUTPUT);

    resetLED();

    // ADC range: 0-4095
    analogReadResolution(12);

    // Allow measurement of higher input voltages.
    analogSetPinAttenuation(COLOR_SENSOR_PIN, ADC_11db);
}

void resetLED() {
    digitalWrite(RED_LED_PIN, LOW);
    digitalWrite(GREEN_LED_PIN, LOW);
    digitalWrite(BLUE_LED_PIN, LOW);
};

int readAverage() {
    long total = 0;

    for (int i = 0; i < NUM_SAMPLES; i++) {
        total += analogRead(COLOR_SENSOR_PIN);
        delayMicroseconds(100);
    }

    return total / NUM_SAMPLES;
};

int readReflection(int ledPin, int ambient) {
    digitalWrite(ledPin, HIGH);
    delay(LED_SETTLE_MS);

    const int illuminated = readAverage();

    digitalWrite(ledPin, LOW);

    return max(0, illuminated - ambient);
};

colorReading single_read () {

    resetLED();

    const int ambient = readAverage();

    const int red = readReflection(RED_LED_PIN, ambient);
    delay(BETWEEN_READINGS_MS);
    
    const int green = readReflection(GREEN_LED_PIN, ambient);
    delay(BETWEEN_READINGS_MS);

    const int blue = readReflection(BLUE_LED_PIN, ambient);
    delay(BETWEEN_READINGS_MS);


    return {red, green, blue};
}

// Main color detection logic

DetectedColor classifyColor(const colorReading& reading) {
    // Reference RGB values:
    // Yellow: R: 65  G: 29  B: 12
    // Blue:   R: 15  G: 16  B: 22
    // Red:    R: 28  G: 7   B: 5
    // Dark:   R: 13  G: 8   B: 5

    int yellow_error =
        (reading.red - 65) * (reading.red - 65) +
        (reading.green - 29) * (reading.green - 29) +
        (reading.blue - 12) * (reading.blue - 12);

    int blue_error =
        (reading.red - 15) * (reading.red - 15) +
        (reading.green - 16) * (reading.green - 16) +
        (reading.blue - 22) * (reading.blue - 22);

    int red_error =
        (reading.red - 28) * (reading.red - 28) +
        (reading.green - 7) * (reading.green - 7) +
        (reading.blue - 5) * (reading.blue - 5);

    int dark_error =
        (reading.red - 13) * (reading.red - 13) +
        (reading.green - 8) * (reading.green - 8) +
        (reading.blue - 5) * (reading.blue - 5);

    // Find minimum error
    int min_error = min(min(yellow_error, blue_error),
                        min(red_error, dark_error));

    if (min_error == yellow_error) return DetectedColor::Yellow;
    if (min_error == blue_error)   return DetectedColor::Blue;
    if (min_error == red_error)    return DetectedColor::Red;

    return DetectedColor::Dark;
}


const char* colorToString(DetectedColor color) {
    switch (color) {
        case DetectedColor::Dark: return "Dark";
        case DetectedColor::Red: return "Red";
        case DetectedColor::Yellow: return "Yellow";
        case DetectedColor::Blue: return "Blue";
        case DetectedColor::White: return "White";
    }

    return "Unknown";
};

void printReading(const colorReading& reading, DetectedColor color) {
    Serial.print("R: ");
    Serial.print(reading.red);

    Serial.print("  G: ");
    Serial.print(reading.green);

    Serial.print("  B: ");
    Serial.print(reading.blue);

    Serial.print("  color: ");
    Serial.println(colorToString(color));
};

