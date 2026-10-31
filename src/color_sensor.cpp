#include "color_sensor.h"
#include "pins.h"
#include <Arduino.h>

constexpr int NUM_SAMPLES = 16;
constexpr int SETTLE_US = 500;
// Measured dark intensity: 0.315-0.401; dimmest lane sample: 1.429.
constexpr float DARK_INTENSITY_THRESHOLD = 0.70f;

struct SensorCalibration {
    float redReference;   
    float yellowGreenReference; 
    float blueReference;  
    float darkR, darkG, darkB;
};

// Color calibration:
// First three values from Raw R on R, Raw G on Y, Raw B on B.
// The final three are R/G/B measured on the dark track background.
constexpr SensorCalibration LEFT_CAL = {68, 147, 136, 32, 45, 42};
constexpr SensorCalibration RIGHT_CAL = {70, 142, 100, 32, 45, 55};

static void resetLED() {
    digitalWrite(RED_LED_PIN, LOW);
    digitalWrite(GREEN_LED_PIN, LOW);
    digitalWrite(BLUE_LED_PIN, LOW);
}

void colorInit() {
    pinMode(RED_LED_PIN, OUTPUT);
    pinMode(GREEN_LED_PIN, OUTPUT);
    pinMode(BLUE_LED_PIN, OUTPUT);
    pinMode(LEFT_COLOR_SENSOR_PIN, INPUT);
    pinMode(RIGHT_COLOR_SENSOR_PIN, INPUT);
    resetLED();
    analogReadResolution(12);
    analogSetPinAttenuation(LEFT_COLOR_SENSOR_PIN, ADC_11db);
    analogSetPinAttenuation(RIGHT_COLOR_SENSOR_PIN, ADC_11db);
}

struct SensorPair { int left, right; };

static SensorPair readAverage() {
    long left = 0, right = 0;
    for (int i = 0; i < NUM_SAMPLES; ++i) {
        left += analogRead(LEFT_COLOR_SENSOR_PIN);
        right += analogRead(RIGHT_COLOR_SENSOR_PIN);
        delayMicroseconds(100);
    }
    return {int(left / NUM_SAMPLES), int(right / NUM_SAMPLES)};
}

static SensorPair readReflection(int ledPin, const SensorPair& ambient) {
    digitalWrite(ledPin, HIGH);
    delayMicroseconds(SETTLE_US);
    const SensorPair illuminated = readAverage();
    digitalWrite(ledPin, LOW);
    return {max(0, illuminated.left - ambient.left), max(0, illuminated.right - ambient.right)};
}

static float calibrate(int raw, float dark, float reference) {
    return reference > dark ? max(0.0f, (raw - dark) / (reference - dark)) : 0.0f;
}

static colorReading calibratedReading(int red, int green, int blue,
                                      const SensorCalibration& cal) {
    const float r = calibrate(red, cal.darkR, cal.redReference);
    const float g = calibrate(green, cal.darkG, cal.yellowGreenReference);
    const float b = calibrate(blue, cal.darkB, cal.blueReference);
    const float total = r + g + b;
    if (total <= 0)
        return {0, 0, 0, red, green, blue, 0};
    return {r / total, g / total, b / total, red, green, blue, total};
}

ColorReadings readColors() {
    resetLED();
    delayMicroseconds(SETTLE_US);
    const SensorPair ambient = readAverage();
    const SensorPair red = readReflection(RED_LED_PIN, ambient);
    const SensorPair green = readReflection(GREEN_LED_PIN, ambient);
    const SensorPair blue = readReflection(BLUE_LED_PIN, ambient);
    resetLED();
    return {calibratedReading(red.left, green.left, blue.left, LEFT_CAL),
            calibratedReading(red.right, green.right, blue.right, RIGHT_CAL)};
}

DetectedColor classifyColor(const colorReading& reading) {
    if (reading.intensity <= DARK_INTENSITY_THRESHOLD)
        return DetectedColor::Dark;

    // Both sensors on yellow: R=0.632-0.699, G=0.290-0.342, B=0.007-0.031.
    // Yellow is red-dominant with this calibration, so R > G alone is not red.
    if (reading.red >= 0.50f && reading.green >= 0.22f && reading.blue <= 0.12f)
        return DetectedColor::Yellow;

    // Left blue is green-dominant: R=0.171-0.188, B=0.327-0.348.
    // Right blue has R=0.092-0.116, B=0.557-0.588. Blue need not exceed G.
    if (reading.red <= 0.25f && reading.green >= 0.20f && reading.blue >= 0.25f)
        return DetectedColor::Blue;

    // Both sensors on red: R=0.761-0.866, G=0.082-0.105, B=0.050-0.135.
    if (reading.red >= 0.70f && reading.green <= 0.20f && reading.blue <= 0.20f)
        return DetectedColor::Red;
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
    Serial.printf("Raw R:%4d G:%4d B:%4d  Normalized R:%5.3f G:%5.3f B:%5.3f  intensity:%5.3f  color:%-6s\n",
                  reading.rawRed, reading.rawGreen, reading.rawBlue,
                  reading.red, reading.green, reading.blue, reading.intensity, colorToString(color));
};

// Return strength = [0,1] of the target color on both sensors, used for lane following
float targetStrength(const colorReading& reading, DetectedColor target) {
    if (target == DetectedColor::Dark || classifyColor(reading) != target) return 0;

    // contrast calculations empirically 
    float contrast = 0;
    switch (target) {
        case DetectedColor::Red:
            contrast = reading.red - max(reading.green, reading.blue); break;
        case DetectedColor::Blue:
            contrast = reading.blue - reading.red; break; // compare blue and red since green will be high
        case DetectedColor::Yellow:
            contrast = 2.0f * (min(reading.red, reading.green) - reading.blue); break; // compare either red or green to blue and scale up reading
        default:
            return 0;
    }
    // Arduino function: contrain(x, a, b) returns a if x<a, b if x>b, and x otherwise
    return constrain(contrast * reading.intensity, 0.0f, 1.0f);
}