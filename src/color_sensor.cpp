#include "color_sensor.h"
#include "pins.h"
#include <Arduino.h>

constexpr int NUM_SAMPLES = 16;
constexpr int DARK_THRESHOLD = 10;
constexpr int WHITE_THRESHOLD = 150;
constexpr float COLOR_DOMINANCE = 1.25f;

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
    
    const int brightest = max(reading.red, max(reading.green, reading.blue));

    if (brightest < DARK_THRESHOLD) return DetectedColor::Dark;

    const bool red =
        reading.red  > 10 && reading.blue < 10 && reading.green < 10;

    const bool blue =
        reading.blue > reading.red * COLOR_DOMINANCE &&
        reading.blue > reading.green * COLOR_DOMINANCE;

    const bool yellow =
        reading.red > reading.blue * COLOR_DOMINANCE &&
        reading.green > reading.blue * COLOR_DOMINANCE;

    if (red) return DetectedColor::Red;
    if (yellow) return DetectedColor::Yellow;
    if (blue) return DetectedColor::Blue;
    if (brightest > WHITE_THRESHOLD) return DetectedColor::White;
    //Dark returned as default
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

