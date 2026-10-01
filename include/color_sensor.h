#pragma once

struct colorReading{
    int red;
    int green;
    int blue;
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
int readReflection(int ledPin, int ambient);
