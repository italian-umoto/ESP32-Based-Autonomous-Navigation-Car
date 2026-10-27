#include "lane_following.h"
#include "color_sensor.h"
#include "drive.h"
#include <Arduino.h>

/* This algorithm assumes that there are two color sensors
 * on the robot and that they are about the width of the lane apart
 */

constexpr float KP = 1.0f;       // TUNABLE
constexpr float KI = 0.0f;       // TUNABLE
constexpr float KD = 0.0f;       // TUNABLE
constexpr float I_LIMIT = 50.0f; // TUNABLE

static float prevErr = 0.0f;
static float integral = 0.0f;

constexpr float MIN_RADIUS_MM = 80.0f; // TUNABLE
constexpr float STRAIGHT_RADIUS_MM = 1e6f;
constexpr float APPROX_STRAIGHT_THRESH = 1e-4f;
constexpr uint32_t LANE_PERIOD_MS = 10; // TUNABLE
constexpr uint32_t SETTLE_MS = 50;      // TUNABLE
constexpr float LOST_THRESHOLD = 0.15f; // TUNABLE

static bool active = false;
static LaneStatus lastStatus = LaneStatus::Lost;
static uint32_t lastStepMs = 0;
static uint32_t startMs = 0;

static void steer(float output) {
  const float mag = fabsf(output);
  const float radius =
      (mag < APPROX_STRAIGHT_THRESH) ? STRAIGHT_RADIUS_MM : MIN_RADIUS_MM / mag;

  if (output > 0)
    drive_turn_left(radius);
  else
    drive_turn_right(radius);
}

static float calcPid(float error) {
  // KI * I_LIMIT gives the maximum correction that can be applied by the
  // integral term. We constrain to keep it within reasonable bounds
  integral = constrain(integral + error, -I_LIMIT, I_LIMIT);

  float output = KP * error + KI * integral + KD * (error - prevErr);
  prevErr = error;
  return constrain(output, -1.0f, 1.0f);
}

void laneStart(DetectedColor color) {
  // Restarting while active must not keep
  // tracking the old color
  if (active)
    laneStop();

  leftColorSensor.startTracking(color);
  rightColorSensor.startTracking(color);

  const uint32_t now = millis();
  startMs = now;
  lastStepMs = now;
  prevErr = 0.0f;
  integral = 0.0f;
  lastStatus = LaneStatus::Following;
  active = true;
}

LaneStatus laneStep(void) {
  if (!active)
    return LaneStatus::Lost;

  // These two cases are here to allow time for the sensors
  // to hit valid output and to normalize when the pid calculation
  // happens. The assumption is made that this code will run frequently
  // enough that we will almost always hit it around every LANE_PERIOD_MS.
  // This allows us to not include dt in our PID calculation because we
  // assume a constant interval.
  const uint32_t now = millis();
  if (now - startMs < SETTLE_MS)
    return lastStatus;
  if (now - lastStepMs < LANE_PERIOD_MS)
    return lastStatus;
  lastStepMs = now;

  // We get values between 0 and 1 from each sensor 0 => black, 1 => color
  const float l = leftColorSensor.lineStrength();
  const float r = rightColorSensor.lineStrength();

  float output;
  if (l + r < LOST_THRESHOLD) {
    // This case occurs when both color sensors hit mostly black,
    // meaning we are not on a line anymore and we should just
    // turn towards wherever we last saw some of the target color.
    // prevErr is intentionally not updated here so the search
    // direction stays fixed until the line is found again
    output = (prevErr >= 0.0f) ? 1.0f : -1.0f;
    lastStatus = LaneStatus::Lost;
  } else {
    // The value passed into calcPid is chosen to create a normalization
    // of the reading that is agnostic to any conditions that equally
    // scale the outputs of the sensors.
    //
    // Let's say the LEDs all dim by the same percentage, readings of
    // 0.75 and 0.25 => 0.6 and 0.2. (l - r)/(l + r) is 0.5 in both cases
    // making the change to the steering the same in both cases which is
    // good because it allows for variation in testing conditions to not
    // require drastically different pid calculations.
    const float error = (l - r) / (l + r);

    // Just reacquired the line: prevErr still holds the old search
    // direction, which would make the D term spike, so we set to current err
    if (lastStatus == LaneStatus::Lost)
      prevErr = error;

    output = calcPid(error);
    lastStatus = LaneStatus::Following;
  }

  steer(output);
  return lastStatus;
}

void laneStop(void) {
  leftColorSensor.stopTracking();
  rightColorSensor.stopTracking();
  drive_stop();
  active = false;
  lastStatus = LaneStatus::Lost;
}
