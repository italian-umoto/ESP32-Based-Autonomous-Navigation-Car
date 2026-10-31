#include "lane_following.h"
#include "drive.h"
#include <Arduino.h>
#include <math.h>

// intensity = sum of calibrated values
// where calibrated = (raw - dark) / (reference - dark)
//
// reference and dark come from color_sensor.cpp calibration, raw has to be
// re-measured
constexpr float FULL_INTENSITY_RED = 1.09f;
constexpr float FULL_INTENSITY_YELLOW = 3.99f;
constexpr float FULL_INTENSITY_BLUE = 1.84f;

constexpr float BASE_CPS = 800;
// how much to slow down while correcting
constexpr float TURN_SLOWDOWN = 0.3f;

constexpr float KP = 600;
constexpr float KD = 100;

// smooths the derivative. 1 = no smoothing, smaller = smoother
constexpr float D_SMOOTHING = 0.2f;

// below this coverage on both sensors the line is lost
constexpr float LOST_COVERAGE = 0.10f;
// error to use while lost
constexpr float LOST_ERROR = 0.6f;

static bool hasPrevious = false;
static float prevError = 0;
static uint32_t prevMs = 0;
static float smoothedRate = 0;

static int lastSide = 0; // +1 = line was last to the left, -1 = right
static bool wasLost = false;

void resetLaneFollowing() {
  hasPrevious = false;
  prevError = 0;
  smoothedRate = 0;
  lastSide = 0;
  wasLost = false;
}

static float fullIntensity(DetectedColor target) {
  switch (target) {
  case DetectedColor::Red:
    return FULL_INTENSITY_RED;
  case DetectedColor::Yellow:
    return FULL_INTENSITY_YELLOW;
  case DetectedColor::Blue:
    return FULL_INTENSITY_BLUE;
  default:
    return 1.0f;
  }
}

// how much of one sensor is on the target line, 0 to 1
static float coverage(const ColorReading &reading, DetectedColor target) {
  // ignore a different lane
  const DetectedColor seen = classifyColor(reading);
  if (seen != target && seen != DetectedColor::Dark)
    return 0;
  return constrain(reading.intensity / fullIntensity(target), 0.0f, 1.0f);
}

// -------------------------------------------------------------------
// PD for lane following (we can add an I term if needed)
//
//   Error       = left coverage - right coverage
//   Correction  = K_p * Error + K_d * (rate of change of Error)
//   Forward     = Base Speed * (1 - slowdown * |Error|)
//   Left speed  = Forward - Correction
//   Right speed = Forward + Correction
// -------------------------------------------------------------------
void performLaneFollowing(DetectedColor target, const ColorReadings &readings) {
  const float left = coverage(readings.left, target);
  const float right = coverage(readings.right, target);

  // remember what side
  float error = left - right;
  if (error > 0.2f)
    lastSide = +1;
  else if (error < -0.2f)
    lastSide = -1;

  // if lost steer back toward the last side
  const bool lost = left < LOST_COVERAGE && right < LOST_COVERAGE;
  if (lost)
    error = lastSide * LOST_ERROR;

  if (lost != wasLost) {
    Serial.println(lost ? "Lane lost, arcing back" : "Lane found");
    wasLost = lost;
  }

  // derivative
  const uint32_t now = millis();
  if (!hasPrevious) {
    hasPrevious = true;
    prevError = error;
    prevMs = now;
  } else if (now != prevMs) {
    const float dt = (now - prevMs) / 1000.0f;
    const float rate = (error - prevError) / dt;
    smoothedRate += D_SMOOTHING * (rate - smoothedRate);
    prevError = error;
    prevMs = now;
  }

  const float forward = BASE_CPS * (1.0f - TURN_SLOWDOWN * fabsf(error));
  const float correction =
      constrain(KP * error + KD * smoothedRate, -forward, forward);

  drive_wheels(forward - correction, forward + correction);
}
