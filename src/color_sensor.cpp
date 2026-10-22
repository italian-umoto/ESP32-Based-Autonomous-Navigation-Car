#include "color_sensor.h"
#include "pins.h"
#include <Arduino.h>

constexpr int NUM_SAMPLES = 16;

// TODO change calibration values if we change chasis or color sensors
// still not really working, color sensors are inconsistent

// LED calibration references
constexpr float WHITE_R = 107.0f;
constexpr float WHITE_G = 55.0f;
constexpr float WHITE_B = 51.0f;
constexpr float DARK_R = 25.0f;
constexpr float DARK_G = 12.0f;
constexpr float DARK_B = 6.0f;
constexpr int DARK_TOTAL_THRESHOLD = 60;

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

int readReflection(int ledPin) {
    resetLED();

    delayMicroseconds(500);
    const int ambient = readAverage();

    digitalWrite(ledPin, HIGH);

    const int illuminated = readAverage();
    digitalWrite(ledPin, LOW);
    return max(0, illuminated - ambient);
};

colorReading single_read () {

    const int red = readReflection(RED_LED_PIN);
    const int green = readReflection(GREEN_LED_PIN);
    const int blue = readReflection(BLUE_LED_PIN);

    // remove each channel's dark offset and scale its white reference to one
    // clamp negative results so readings below each dark reference stay at 0
    const float calibratedRed = max(0.0f, (red - DARK_R) / (WHITE_R - DARK_R));
    const float calibratedGreen = max(0.0f, (green - DARK_G) / (WHITE_G - DARK_G));
    const float calibratedBlue = max(0.0f, (blue - DARK_B) / (WHITE_B - DARK_B));
    const float total = calibratedRed + calibratedGreen + calibratedBlue;
    if (total == 0.0f) return {0.0f, 0.0f, 0.0f, red, green, blue};

    return {calibratedRed / total, calibratedGreen / total, calibratedBlue / total,
            red, green, blue};
};

DetectedColor classifyColor(const colorReading& reading){
    
    const int totalRaw = reading.rawRed + reading.rawGreen + reading.rawBlue;
    if (totalRaw <= DARK_TOTAL_THRESHOLD) return DetectedColor::Dark;

    // approx equal normalized channels
    if (reading.red >= 0.25f && reading.red <= 0.42f &&
        reading.green >= 0.25f && reading.green <= 0.42f &&
        reading.blue >= 0.25f && reading.blue <= 0.42f)
        return DetectedColor::White;

    if (reading.blue >= 0.40f && reading.blue > reading.red &&
        reading.blue > reading.green)
        return DetectedColor::Blue;

    if (reading.blue <= 0.20f && reading.red >= 0.48f &&
        reading.red > reading.green * 1.35f)
        return DetectedColor::Red;

    if (reading.blue <= 0.20f && reading.red >= 0.34f &&
        reading.green >= 0.32f && reading.red <= reading.green * 1.35f)
        return DetectedColor::Yellow;

    return DetectedColor::Dark;
};

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
    Serial.printf("Raw R:%4d G:%4d B:%4d  Normalized R:%5.3f G:%5.3f B:%5.3f  color:%-6s\n",
                  reading.rawRed, reading.rawGreen, reading.rawBlue,
                  reading.red, reading.green, reading.blue, colorToString(color));
};

