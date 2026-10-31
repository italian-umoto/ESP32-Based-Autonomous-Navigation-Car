#pragma once

struct colorReading{
    float red;
    float green;
    float blue;
    int rawRed;
    int rawGreen;
    int rawBlue;
    float intensity; // Sum of calibrated channels before chromatic normalization.
};

enum class DetectedColor{
    Dark,
    Red,
    Yellow,
    Blue,
    White
};

void colorInit();
struct ColorReadings {
    colorReading left;
    colorReading right;
};

ColorReadings readColors();
float targetStrength(const colorReading& reading, DetectedColor target);
DetectedColor classifyColor(const colorReading& reading);
const char* colorToString(DetectedColor color);
void printReading(const colorReading& reading, DetectedColor color);
