#include "color_sensor.h"
#include <Arduino.h>

// Lets see if we can throw everything on ADC 1 if possible
// I believe that ADC 2 pins can interfere with WiFi
constexpr int L_SENSOR_PIN = 6;
constexpr int L_RED_LED_PIN = 16;
constexpr int L_GREEN_LED_PIN = 11;
constexpr int L_BLUE_LED_PIN = 12;

// TEMP: idk what pins are free
constexpr int R_SENSOR_PIN = 10;
constexpr int R_RED_LED_PIN = 16;
constexpr int R_GREEN_LED_PIN = 11;
constexpr int R_BLUE_LED_PIN = 12;

// These values get updated during testing
constexpr SensorCal LEFT_CAL = {{32, 68}, {45, 147}, {42, 136}};
constexpr SensorCal RIGHT_CAL = {{32, 70}, {45, 142}, {55, 100}};

constexpr int NUM_SAMPLES = 16;
constexpr int DARK_THRESHOLD = 25;
constexpr int WHITE_THRESHOLD = 150;
constexpr float COLOR_DOMINANCE = 1.1f;

constexpr int LED_SETTLE_MS = 10;
constexpr int BETWEEN_READINGS_MS = 10;

// Here is the birth of the glorious color sensors
ColorSensor leftColorSensor(L_SENSOR_PIN, L_RED_LED_PIN, L_GREEN_LED_PIN,
                            L_BLUE_LED_PIN, LEFT_CAL);

ColorSensor rightColorSensor(R_SENSOR_PIN, R_RED_LED_PIN, R_GREEN_LED_PIN,
                             R_BLUE_LED_PIN, RIGHT_CAL);

void setupColorSensors() {
  leftColorSensor.colorSensorInit();
  rightColorSensor.colorSensorInit();
}

bool bothSensorsSee(DetectedColor target) {
  if (leftColorSensor.isTracking() || rightColorSensor.isTracking()) {
    Serial.println(
        "bothSensorsSee: sensor still tracking, call laneStop() first");
    return false;
  }

  const DetectedColor left =
      leftColorSensor.classifyColor(leftColorSensor.singleRead());
  if (left != target)
    return false;

  const DetectedColor right =
      rightColorSensor.classifyColor(rightColorSensor.singleRead());
  return right == target;
}

ColorSensor::ColorSensor(int sensorPin, int redPin, int greenPin, int bluePin,
                         const SensorCal &cal)
    : m_sensor_pin(sensorPin), m_red_pin(redPin), m_green_pin(greenPin),
      m_blue_pin(bluePin), m_cal(cal) {}

void ColorSensor::colorSensorInit() {
  pinMode(m_red_pin, OUTPUT);
  pinMode(m_green_pin, OUTPUT);
  pinMode(m_blue_pin, OUTPUT);

  resetLED();

  analogRead(m_sensor_pin);
  delay(10);
  // ADC range in bits: 0-4095
  analogReadResolution(12);
  // Allow measurement of higher input voltages.
  analogSetPinAttenuation(m_sensor_pin, ADC_11db);
}

void ColorSensor::resetLED() {
  digitalWrite(m_red_pin, LOW);
  digitalWrite(m_green_pin, LOW);
  digitalWrite(m_blue_pin, LOW);
}

int ColorSensor::readAverage() {
  long total = 0;

  for (int i = 0; i < NUM_SAMPLES; i++) {
    total += analogRead(m_sensor_pin);
    delayMicroseconds(100);
  }

  return total / NUM_SAMPLES;
}

int ColorSensor::readReflection(int ledPin, int ambient) {
  digitalWrite(ledPin, HIGH);
  delay(LED_SETTLE_MS);

  const int illuminated = readAverage();

  digitalWrite(ledPin, LOW);

  return max(0, illuminated - ambient);
}

colorReading ColorSensor::singleRead() {
  resetLED();

  if (m_tracking)
    delay(50);

  const int ambient = readAverage();

  const int red = readReflection(m_red_pin, ambient);
  delay(BETWEEN_READINGS_MS);

  const int green = readReflection(m_green_pin, ambient);
  delay(BETWEEN_READINGS_MS);

  const int blue = readReflection(m_blue_pin, ambient);

  if (m_tracking)
    digitalWrite(ledPinFor(m_target), HIGH);

  return {red, green, blue};
}

// Main color detection logic
DetectedColor ColorSensor::classifyColor(const colorReading &reading) {
  constexpr int DARK_MAX = 87;    // TUNABLE: darks peak ≤ ~78, red ≥ ~94
  constexpr int DARK_SPREAD = 20; // TUNABLE: catches bright, flat darks

  const int hi = max(reading.red, max(reading.green, reading.blue));
  const int lo = min(reading.red, min(reading.green, reading.blue));

  // Dark if nothing is bright, or everything is about equally bright
  if (hi < DARK_MAX || hi - lo < DARK_SPREAD)
    return DetectedColor::Dark;

  // Blue barely reflects red; red and yellow reflect it strongly
  if (reading.blue > reading.red)
    return DetectedColor::Blue;

  // Yellow reflects more green than red; red doesn't
  if (reading.green > reading.red)
    return DetectedColor::Yellow;

  return DetectedColor::Red;
}

static const char *colorToString(DetectedColor color) {
  switch (color) {
  case DetectedColor::Dark:
    return "Dark";
  case DetectedColor::Red:
    return "Red";
  case DetectedColor::Yellow:
    return "Yellow";
  case DetectedColor::Blue:
    return "Blue";
  case DetectedColor::White:
    return "White";
  }
  return "Unknown";
}

void ColorSensor::testReading() {
  const colorReading reading = singleRead();
  const DetectedColor color = classifyColor(reading);

  printReading(reading, color);
}

void ColorSensor::printReading(const colorReading &reading,
                               DetectedColor color) {
  Serial.print("R: ");
  Serial.print(reading.red);
  Serial.print("  G: ");
  Serial.print(reading.green);
  Serial.print("  B: ");
  Serial.print(reading.blue);
  Serial.print("  color: ");
  Serial.println(colorToString(color));
}

void ColorSensor::startTracking(DetectedColor target) {
  const int pin = ledPinFor(target);
  if (pin < 0)
    return;

  resetLED();
  digitalWrite(pin, HIGH);

  m_target = target;
  m_tracking = true;
}

void ColorSensor::stopTracking() {
  resetLED();
  m_tracking = false;
}

void ColorSensor::testTracking(DetectedColor target) {
  if (!m_tracking || m_target != target) {
    startTracking(target);
    delay(50);
  }

  Serial.print("raw: ");
  Serial.print(readAverage());
  Serial.print("  strength: ");
  Serial.println(lineStrength());
}

float ColorSensor::lineStrength() {
  const ChannelCal *c = calFor(m_target);
  if (!m_tracking || c == nullptr || c->line == c->black)
    return 0.0f;

  const float s = float(readAverage() - c->black) / float(c->line - c->black);

  return constrain(s, 0.0f, 1.0f);
}

int ColorSensor::ledPinFor(DetectedColor target) const {
  switch (target) {
  case DetectedColor::Red:
    return m_red_pin;
  case DetectedColor::Yellow:
    return m_green_pin;
  case DetectedColor::Blue:
    return m_blue_pin;
  default:
    return -1;
  }
}

const ChannelCal *ColorSensor::calFor(DetectedColor target) const {
  switch (target) {
  case DetectedColor::Red:
    return &m_cal.red;
  case DetectedColor::Yellow:
    return &m_cal.yellow;
  case DetectedColor::Blue:
    return &m_cal.blue;
  case DetectedColor::Dark:
    return nullptr;
  case DetectedColor::White:
    return nullptr;
  }
  return nullptr;
}
