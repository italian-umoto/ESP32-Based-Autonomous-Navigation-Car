#include "lane_following.h"
#include "drive.h"
#include <Arduino.h>

// robot straddles lane between two sensors
constexpr float BASE_CPS = 300;
constexpr float KP = 150;
// optional limit on steering, not currently used (see commented line below)
constexpr float MAX_CORRECTION = 80;

// K_p lane following: https://www.youtube.com/watch?v=ie2q1gr6Y7M
//   Error       = left strength - right strength
//   Correction  = K_p * Error
//   Left speed  = Base Speed - Correction
//   Right speed = Base Speed + Correction
// So if the left sensor sees more of the lane, the robot steers left
void performLaneFollowing(DetectedColor target, const ColorReadings &readings) {
  const float left = targetStrength(readings.left, target);
  // Right sensor's blue reading is a bit weaker; may need a higher weight
  const float right = targetStrength(readings.right, target);

  const float error = left - right;
  const float correction = KP * error;
  // const float correction = constrain(KP * error, -MAX_CORRECTION,
  // MAX_CORRECTION);

  drive_wheels(BASE_CPS - correction, BASE_CPS + correction);
}
