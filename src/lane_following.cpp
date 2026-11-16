#include "lane_following.h"
#include "drive.h"
#include <Arduino.h>
#include <math.h>

// Color readings are ambient-subtracted ADC counts.
// targetStrength() converts color contrast into a 0-1
// strength value and ignores non-target colors.

constexpr float BASE_CPS = 800;

// How much to slow down while correcting
constexpr float TURN_SLOWDOWN = 0.3f;

constexpr float KP = 600;
constexpr float KD = 100;

// Smooths the derivative.
// 1 = no smoothing, smaller = smoother
constexpr float D_SMOOTHING = 0.2f;

// Below this coverage on both sensors, lane is lost
constexpr float LOST_COVERAGE = 0.10f;

// Steering error when the lane is lost
constexpr float LOST_ERROR = 0.6f;

static bool hasPrevious = false;
static float prevError = 0;
static uint32_t prevMs = 0;
static float smoothedRate = 0;

static int lastSide = 0;
static bool wasLost = false;

void resetLaneFollowing() {
  hasPrevious = false;
  prevError = 0;
  smoothedRate = 0;
  lastSide = 0;
  wasLost = false;
}

// NEW: Use the color sensor's target strength.
// Returns 0 for Dark or any other non-target color.
static float coverage(const ColorReading &reading,
                      DetectedColor target) {
  return targetStrength(reading, target);
}

// ----------------------------------------------------
// PD lane following
// ----------------------------------------------------

void performLaneFollowing(DetectedColor target,
                          const ColorReadings &readings) {

  const float left = coverage(readings.left, target);
  const float right = coverage(readings.right, target);

  // Positive error: target stronger on left.
  // Negative error: target stronger on right.
  float error = left - right;

  // Remember which side the lane was last seen
  if (error > 0.2f)
    lastSide = +1;
  else if (error < -0.2f)
    lastSide = -1;

  // If both sensors lose the line,
  // steer toward the last known side.
  const bool lost =
      left < LOST_COVERAGE &&
      right < LOST_COVERAGE;

  if (lost)
    error = lastSide * LOST_ERROR;

  if (lost != wasLost) {
    Serial.println(
      lost ? "Lane lost, arcing back" : "Lane found"
    );
    wasLost = lost;
  }

  // Derivative calculation
  const uint32_t now = millis();

  if (!hasPrevious) {
    hasPrevious = true;
    prevError = error;
    prevMs = now;
  }
  else if (now != prevMs) {
    const float dt = (now - prevMs) / 1000.0f;
    const float rate = (error - prevError) / dt;

    smoothedRate +=
        D_SMOOTHING * (rate - smoothedRate);

    prevError = error;
    prevMs = now;
  }

  // Slow down when steering
  const float forward =
      BASE_CPS *
      (1.0f - TURN_SLOWDOWN * fabsf(error));

  // PD correction
  const float correction =
      constrain(
        KP * error + KD * smoothedRate,
        -forward,
        forward
      );

  // Differential wheel control
  drive_wheels(
    forward - correction,
    forward + correction
  );
}
