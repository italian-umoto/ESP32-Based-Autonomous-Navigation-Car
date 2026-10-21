#include "color_sensor.h"
#include "pins.h"
#include <Arduino.h>

constexpr int NUM_SAMPLES = 16;
constexpr float EMA_ALPHA = 0.5f;

static bool emaInitialized = false;
static float smoothedRed = 0.0f;
static float smoothedGreen = 0.0f;
static float smoothedBlue = 0.0f;

// TODO change calibration values if we change chasis or color sensors
// still not really working, color sensors are too inconsistent

// LED calibration references
constexpr float WHITE_R = 107.0f;
constexpr float WHITE_G = 55.0f;
constexpr float WHITE_B = 51.0f;
constexpr float DARK_R = 25.0f;
constexpr float DARK_G = 12.0f;
constexpr float DARK_B = 6.0f;
constexpr int DARK_TOTAL_THRESHOLD = 60;

void colorInit() {
    emaInitialized = false;
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

    delayMicroseconds(2000);
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

    if (!emaInitialized) {
        smoothedRed = red;
        smoothedGreen = green;
        smoothedBlue = blue;
        emaInitialized = true;
    } else { // exponential moving average
        smoothedRed = EMA_ALPHA * red + (1.0f - EMA_ALPHA) * smoothedRed;
        smoothedGreen = EMA_ALPHA * green + (1.0f - EMA_ALPHA) * smoothedGreen;
        smoothedBlue = EMA_ALPHA * blue + (1.0f - EMA_ALPHA) * smoothedBlue;
    }

    // remove each channel's dark offset and scale its white reference to one
    // clamp negative results so readings below each dark reference stay at 0
    const float calibratedRed = max(0.0f, (smoothedRed - DARK_R) / (WHITE_R - DARK_R));
    const float calibratedGreen = max(0.0f, (smoothedGreen - DARK_G) / (WHITE_G - DARK_G));
    const float calibratedBlue = max(0.0f, (smoothedBlue - DARK_B) / (WHITE_B - DARK_B));
    const float total = calibratedRed + calibratedGreen + calibratedBlue;
    if (total == 0.0f) return {0.0f, 0.0f, 0.0f, red, green, blue,
                              smoothedRed, smoothedGreen, smoothedBlue};

    return {calibratedRed / total, calibratedGreen / total, calibratedBlue / total,
            red, green, blue, smoothedRed, smoothedGreen, smoothedBlue};
};

DetectedColor classifyColor(const colorReading& reading){
    
    const float totalSmoothed = reading.smoothedRed + reading.smoothedGreen + reading.smoothedBlue;
    if (totalSmoothed <= DARK_TOTAL_THRESHOLD) return DetectedColor::Dark;

    // ~equal normalized channels
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
    Serial.printf("Raw R:%4d G:%4d B:%4d  Smoothed R:%6.1f G:%6.1f B:%6.1f  Normalized R:%5.3f G:%5.3f B:%5.3f  color:%-6s\n",
                  reading.rawRed, reading.rawGreen, reading.rawBlue,
                  reading.smoothedRed, reading.smoothedGreen, reading.smoothedBlue,
                  reading.red, reading.green, reading.blue, colorToString(color));
};

