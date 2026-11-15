#include "color_sensor.h"
#include "pins.h"
#include <Arduino.h>

constexpr int NUM_SAMPLES = 16;  // ADC reads averaged per measurement
constexpr int SETTLE_US = 15000; // wait after switching an LED

constexpr int LANE_MIN_G = 100;
constexpr float YELLOW_BLUE_SPLIT = 0.60f;
constexpr float YELLOW_MAX_B = 0.35f; // B/G on yellow: 0.06-0.15
constexpr float BLUE_MIN_B = 0.40f;   // B/G on blue: 0.49-0.88

// Red vs black. Red only lifts R, so it is detected by shape, not brightness:
// R / max(G, B) is 1.0-1.44 on every shade of black and 1.67-2.45 on red.
constexpr float RED_RATIO = 1.60f;
// A glitch that drops G and B by ~16 counts makes black pass the ratio
// (58/33/16), but R stays at the black level. Real red raises R 16-80 counts
// above the black baseline, so require some of that too.
constexpr float RED_MIN_EXCESS = 12.0f;
// keeps the ratio meaningful when G and B are near 0
constexpr int RATIO_FLOOR = 10;

// Running black baseline, per sensor. Only readings that look like black
// update it, so it follows lighter/darker floor but not the lanes.
constexpr float BASELINE_ALPHA = 0.25f;     // ~1 s time constant at 5 reads/s
constexpr float BASELINE_MAX_RATIO = 1.45f; // just above black's worst 1.44

struct SensorCalibration {
  float redReference, yellowGreenReference, blueReference, darkR, darkG, darkB;
};

// First three values from Raw R on R, Raw G on Y, Raw B on B.
// The final three are R/G/B measured on the dark track background.
constexpr SensorCalibration LEFT_CAL = {53.0f, 93.0f, 155.0f,
                                        28.0f, 24.0f, 87.0f};
constexpr SensorCalibration RIGHT_CAL = {51.0f, 88.0f, 84.0f,
                                         13.0f, 19.0f, 22.0f};

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
  return {int(left / NUM_SAMPLES), int(right / NUM_SAMPLES)};
}

static SensorPair readReflection(int ledPin, const SensorPair &ambient) {
  digitalWrite(ledPin, HIGH);
  delayMicroseconds(SETTLE_US);
  const SensorPair illuminated = readAverage();
  digitalWrite(ledPin, LOW);
  return {max(0, illuminated.left - ambient.left),
          max(0, illuminated.right - ambient.right)};
}

static ColorReading calibratedReading(int red, int green, int blue,
                                      const SensorCalibration &cal) {
  const float r = max(0.0f, (red - cal.darkR) / (cal.redReference - cal.darkR));
  const float g =
      max(0.0f, (green - cal.darkG) / (cal.yellowGreenReference - cal.darkG));
  const float b =
      max(0.0f, (blue - cal.darkB) / (cal.blueReference - cal.darkB));
  const float total = r + g + b;
  // when all three readings are below the corresponding darkX threshold
  if (total <= 0)
    return {0, 0, 0, red, green, blue, 0};
  return {r / total, g / total, b / total, red, green, blue, total};
}

////////////////////////////////////////////////////////////////////

struct BlackBaseline {
  float r, g, b;
  bool valid;
};
static BlackBaseline leftBlack = {0, 0, 0, false};
static BlackBaseline rightBlack = {0, 0, 0, false};

static float ratioDenominator(float g, float b) {
  return max(max(g, b), float(RATIO_FLOOR));
}

static bool looksLikeBlack(int r, int g, int b) {
  return g < LANE_MIN_G && r < BASELINE_MAX_RATIO * ratioDenominator(g, b);
}

static void applyBlackBaseline(ColorReading &reading, BlackBaseline &black) {
  const int r = reading.rawRed, g = reading.rawGreen, b = reading.rawBlue;

  if (black.valid) {
    reading.blackRed = black.r;
    reading.blackGreen = black.g;
    reading.blackBlue = black.b;
  } else {
    // no black seen yet: zero excess, so nothing can classify as Red
    reading.blackRed = r;
    reading.blackGreen = g;
    reading.blackBlue = b;
  }

  if (!looksLikeBlack(r, g, b))
    return;
  if (!black.valid) {
    black = {float(r), float(g), float(b), true};
    return;
  }
  black.r += BASELINE_ALPHA * (r - black.r);
  black.g += BASELINE_ALPHA * (g - black.g);
  black.b += BASELINE_ALPHA * (b - black.b);
}

void resetBlackBaseline() { leftBlack.valid = rightBlack.valid = false; }

ColorReadings readColors() {
  resetLED();
  delayMicroseconds(SETTLE_US);
  const SensorPair ambient = readAverage();
  const SensorPair red = readReflection(RED_LED_PIN, ambient);
  const SensorPair green = readReflection(GREEN_LED_PIN, ambient);
  const SensorPair blue = readReflection(BLUE_LED_PIN, ambient);
  resetLED();
  ColorReadings readings = {
      calibratedReading(red.left, green.left, blue.left, LEFT_CAL),
      calibratedReading(red.right, green.right, blue.right, RIGHT_CAL)};
  applyBlackBaseline(readings.left, leftBlack);
  applyBlackBaseline(readings.right, rightBlack);
  return readings;
}

DetectedColor classifyColor(const ColorReading &reading) {
  const float r = reading.rawRed, g = reading.rawGreen, b = reading.rawBlue;

  // yellow and blue are far brighter than any black on the green channel
  if (g >= LANE_MIN_G) {
    if (r >= YELLOW_BLUE_SPLIT * g && b <= YELLOW_MAX_B * g)
      return DetectedColor::Yellow;
    if (r < YELLOW_BLUE_SPLIT * g && b >= BLUE_MIN_B * g)
      return DetectedColor::Blue;
    return DetectedColor::Dark;
  }

  // red: R well ahead of G and B, and R actually above the local black
  if (r >= RED_RATIO * ratioDenominator(g, b) &&
      r - reading.blackRed >= RED_MIN_EXCESS)
    return DetectedColor::Red;

  return DetectedColor::Dark;
}

// strength in [0, 1] of the target color on one sensor, used for lane
// following. 0 if the sensor isn't on that color at all
float targetStrength(const ColorReading &reading, DetectedColor target) {
  if (target == DetectedColor::Dark || classifyColor(reading) != target)
    return 0;

  float contrast = 0;
  switch (target) {
  case DetectedColor::Red:
    contrast = reading.red - max(reading.green, reading.blue);
    break;
  case DetectedColor::Blue:
    // compare blue to red, since green is also high on blue
    contrast = reading.blue - reading.red;
    break;
  case DetectedColor::Yellow:
    // compare the weaker of red/green to blue, scaled up
    contrast = 2.0f * (min(reading.red, reading.green) - reading.blue);
    break;
  default:
    return 0;
  }
  return constrain(contrast * reading.intensity, 0.0f, 1.0f);
}

// -------------------------------------------------------------------
// debug
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

void printReading(const ColorReading &reading, DetectedColor color) {
  //   Serial.printf("Raw R:%4d G:%4d B:%4d  Normalized R:%5.3f G:%5.3f B:%5.3f
  //   "
  //                 "intensity:%5.3f  color:%-6s\n",
  //                 reading.rawRed, reading.rawGreen, reading.rawBlue,
  //                 reading.red, reading.green, reading.blue,
  //                 reading.intensity, colorToString(color));
  Serial.printf(
      "Normalized R:%5.3f G:%5.3f B:%5.3f intensity:%5.3f color:%-6s\n",
      reading.red, reading.green, reading.blue, reading.intensity,
      colorToString(color));
}
