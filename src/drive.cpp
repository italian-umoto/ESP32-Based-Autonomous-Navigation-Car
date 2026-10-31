#include "drive.h"
#include "motors.h"
#include <math.h>
#include <stdlib.h>

static constexpr float LEFT_WHEEL_DIR = -1;
static constexpr float RIGHT_WHEEL_DIR = 1;
static constexpr float ARC_TURN_CPS = 200;

// drive_turn: encoder counts per wheel for a 90 degree turn (measured)
static constexpr int COUNTS_PER_90 = 1000;
// drive_turn: wheel speed while turning in place
static constexpr float TURN_IN_PLACE_CPS = 600;

void drive_wheels(float left_cps, float right_cps) {
  // scale both down together so neither goes over the max
  const float peak = fmaxf(fabsf(left_cps), fabsf(right_cps));
  if (peak > DRIVE_MAX_WHEEL_CPS) {
    const float k = DRIVE_MAX_WHEEL_CPS / peak;
    left_cps *= k;
    right_cps *= k;
  }

  left_cps *= LEFT_WHEEL_DIR;
  right_cps *= RIGHT_WHEEL_DIR;
  motors_command(left_cps, right_cps);
}

void drive_stop(void) { motors_stop(); }
void drive_forward(float speed) {
  speed = fabsf(speed);
  drive_wheels(speed, speed);
}
void drive_backward(float speed) {
  speed = fabsf(speed);
  drive_wheels(-speed, -speed);
}
void drive_pivot_cw(float speed) {
  speed = fabsf(speed);
  drive_wheels(speed, -speed);
}
void drive_pivot_ccw(float speed) {
  speed = fabsf(speed);
  drive_wheels(-speed, speed);
}

// the outer wheel travels a bigger circle than the inner one, so it has to
// go faster
// each wheel's speed scales with the radius of its own circle
//
// https://introtoroboticsv2.readthedocs.io/en/latest/course/driving/differential_steering.html
static void drive_arc(float speed, float radius_mm, bool right) {
  speed = fabsf(speed);
  radius_mm = fabsf(radius_mm);

  if (radius_mm < 1.0f) {
    right ? drive_pivot_cw(speed) : drive_pivot_ccw(speed);
    return;
  }

  const float half = DRIVE_TRACK_WIDTH_MM / 2.0f;
  const float outer = speed * (radius_mm + half) / radius_mm;
  const float inner = speed * (radius_mm - half) / radius_mm;

  if (right)
    drive_wheels(outer, inner);
  else
    drive_wheels(inner, outer);
}

void drive_turn_right(float radius_mm) {
  drive_arc(ARC_TURN_CPS, radius_mm, true);
}
void drive_turn_left(float radius_mm) {
  drive_arc(ARC_TURN_CPS, radius_mm, false);
}

// WITH DEGREES
bool drive_turn(int degrees, int startLeft, int startRight) {
  const int target = COUNTS_PER_90 * abs(degrees) / 90;
  const int left = abs(left_encoder_count() - startLeft);
  const int right = abs(right_encoder_count() - startRight);

  if (left >= target && right >= target) {
    drive_stop();
    return true;
  }

  // stop each wheel at its target while the other finishes the pivot
  const float leftSpeed =
      left >= target ? 0
                     : (degrees > 0 ? TURN_IN_PLACE_CPS : -TURN_IN_PLACE_CPS);
  const float rightSpeed =
      right >= target ? 0
                      : (degrees > 0 ? -TURN_IN_PLACE_CPS : TURN_IN_PLACE_CPS);
  drive_wheels(leftSpeed, rightSpeed);
  return false;
}
