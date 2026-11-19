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

  // YELLOW
  if (r >= 650 && g >= 700) {
    return DetectedColor::Yellow;
  }

  // BLUE
  if (g >= 550 && b >= 575) {
    return DetectedColor::Blue;
  }

  // RED
  if (r >= 250 &&
      g <= 600 &&
      b <= 570) {
    return DetectedColor::Red;
  }

  // DARK
  return DetectedColor::Dark;
}


float targetStrength(const ColorReading &reading,
                     DetectedColor target) {

  if (target == DetectedColor::Dark ||
      classifyColor(reading) != target) {
    return 0.0f;
  }

  float strength = 0.0f;

  switch (target) {
    case DetectedColor::Red:
      strength = (reading.red - 108.0f) / 283.0f;
      break;

    case DetectedColor::Blue:
      strength = (reading.red - 108.0f) / 65.0f;
      break;

    case DetectedColor::Yellow:
      strength = (reading.red - 108.0f) / 737.0f;
      break;

    default:
      return 0.0f;
  }

  return constrain(strength, 0.0f, 1.0f);
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
