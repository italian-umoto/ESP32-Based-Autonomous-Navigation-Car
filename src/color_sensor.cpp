#include "color_sensor.h"
#include "pins.h"
#include <Arduino.h>

constexpr int NUM_SAMPLES = 16; // ADC reads averaged per measurement
constexpr int SETTLE_US = 500;  // wait after switching an LED

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

struct SensorPair {
  int left, right;
};

static SensorPair readAverage() {

  long left = 0, right = 0;

  for (int i = 0; i < NUM_SAMPLES; ++i) {
    left += analogRead(LEFT_COLOR_SENSOR_PIN);
    right += analogRead(RIGHT_COLOR_SENSOR_PIN);
    delayMicroseconds(100);
  }

  return {
    int(left / NUM_SAMPLES),
    int(right / NUM_SAMPLES)
  };

}

// Measure and subtract ambient separately for each LED
static SensorPair readReflection(int ledPin) {

  resetLED();
  delayMicroseconds(SETTLE_US);

  const SensorPair ambient = readAverage();

  digitalWrite(ledPin, HIGH);
  delayMicroseconds(SETTLE_US);

  const SensorPair illuminated = readAverage();

  digitalWrite(ledPin, LOW);

  return {
    max(0, illuminated.left - ambient.left),
    max(0, illuminated.right - ambient.right)
  };
}

static ColorReading rawReading(int red, int green, int blue) {

  const float intensity = float(red + green + blue);

  return {float(red), float(green), float(blue), red, green, blue, intensity};

}

ColorReadings readColors() {

  const SensorPair red = readReflection(RED_LED_PIN);
  const SensorPair green = readReflection(GREEN_LED_PIN);
  const SensorPair blue = readReflection(BLUE_LED_PIN);

  resetLED();

  return {
    rawReading(red.left, green.left, blue.left),
    rawReading(red.right, green.right, blue.right)
  };
}

DetectedColor classifyColor(const ColorReading &reading) {
  const int r = reading.rawRed;
  const int g = reading.rawGreen;
  const int b = reading.rawBlue;

  // STRONG YELLOW
  if (r >= 160 &&
      g >= 135 &&
      b <= 50) {
    return DetectedColor::Yellow;
  }

  // WEAK YELLOW
  if (r >= 100 &&
      g >= 135 &&
      b <= 45 &&
      g - b >= 110) {
    return DetectedColor::Yellow;
  }

  // BLUE
  if (r <= 75 &&
      (g >= 85 || (g >= 50 && b >= 34))) {
    return DetectedColor::Blue;
  }

  // STRONG RED
  if (r >= 90 &&
      g <= 80 &&
      b <= 32) {
    return DetectedColor::Red;
  }

  // WEAK RED
  if (r >= 46 &&
      r < 90 &&
      g <= 80 &&
      b <= 32) {
    return DetectedColor::Red;
  }

  // VERY WEAK RED
  if (r >= 15 &&
      r <= 45 &&
      g <= 28 &&
      b >= 1 &&
      b <= 28 &&
      (r >= 35 ||
      (g <= 5 && b <= 5) ||
      b >= 19)) {
    return DetectedColor::Red;
  }

  // DARK
  return DetectedColor::Dark;
}


// Target strength (0 to 1) for lane following
float targetStrength(const ColorReading &reading, DetectedColor target) {
  
  if (target == DetectedColor::Dark || classifyColor(reading) != target)
    return 0.0f;

  float contrast = 0.0f;
  float scale = 1.0f;

  switch (target) {
  case DetectedColor::Red:
    // Red is stronger than green and blue.
    contrast = reading.red - max(reading.green, reading.blue);
    scale = 60.0f;
    break;

  case DetectedColor::Blue:
    // Blue surfaces produce strong green reflection.
    contrast = reading.green - reading.red;
    scale = 140.0f;
    break;

  case DetectedColor::Yellow:
    // Red and green are both strong compared to blue.
    contrast = min(reading.red, reading.green) - reading.blue;
    scale = 230.0f;
    break;

  default:
    return 0.0f;
  }

  return constrain(contrast / scale, 0.0f, 1.0f);
}

// -------------------------------------------------------------------
// Debug
// -------------------------------------------------------------------

const char *colorToString(DetectedColor color) {
  switch (color) {
  case DetectedColor::Dark:
    return "Dark";
  case DetectedColor::Red:
    return "Red";
  case DetectedColor::Yellow:
    return "Yellow";
  case DetectedColor::Blue:
    return "Blue";
  }

  return "Unknown";
}

void printReading(const ColorReading &reading,
                  DetectedColor color) {
  Serial.printf(
    "R:%d G:%d B:%d intensity:%5.3f color:%-6s\n",
    reading.rawRed,
    reading.rawGreen,
    reading.rawBlue,
    reading.intensity,
    colorToString(color)
  );
}
