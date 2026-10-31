#pragma once

struct ColorReading {
  float red;
  float green;
  float blue;
  int rawRed; // ambient-subtracted ADC value under the red LED
  int rawGreen;
  int rawBlue;
  float intensity;
};

struct ColorReadings {
  ColorReading left;
  ColorReading right;
};

enum class DetectedColor {
  Dark,
  Red,
  Yellow,
  Blue,
  White // not currently returned by classifyColor
};

void colorInit();
ColorReadings readColors();
// which color one sensor is on
DetectedColor classifyColor(const ColorReading &reading);

// gives strength reading 0-1 for lane following
float targetStrength(const ColorReading &reading, DetectedColor target);

// debug
const char *colorToString(DetectedColor color);
void printReading(const ColorReading &reading, DetectedColor color);
