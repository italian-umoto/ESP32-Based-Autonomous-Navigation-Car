#pragma once

struct colorReading{
    float red;
    float green;
    float blue;
    int rawRed;
    int rawGreen;
    int rawBlue;
    float smoothedRed;
    float smoothedGreen;
    float smoothedBlue;
};

enum class DetectedColor{
    Dark,
    Red,
    Yellow,
    Blue,
    White
};

void colorInit();
colorReading single_read ();
DetectedColor classifyColor(const colorReading& reading);
const char* colorToString(DetectedColor color);
void printReading(const colorReading& reading, DetectedColor color);
void resetLED();
int readAverage();
int readReflection(int ledPin);
