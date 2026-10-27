#include "drive.h"
#include "motors.h"
#include <math.h>

#define LEFT_WHEEL_DIR -1
#define RIGHT_WHEEL_DIR 1

#define DRIVE_TURN_SPEED 1000

#define DRIVE_COUNTS_PER_DEG (850.0f / 90.0f) // Found by Justin

static struct {
  bool active = false;
  int32_t startL, startR;
  int32_t target;
  float speed;
} s_rot;

static inline void rotate_cancel(void) { s_rot.active = false; }

// Scale down the inputs to clamp in the max CPS
static void drive_wheels(float left_cps, float right_cps) {
  float peak = fmaxf(fabsf(left_cps), fabsf(right_cps));
  if (peak > DRIVE_MAX_WHEEL_CPS) {
    float k = DRIVE_MAX_WHEEL_CPS / peak;
    left_cps *= k;
    right_cps *= k;
  }

  left_cps *= LEFT_WHEEL_DIR;
  right_cps *= RIGHT_WHEEL_DIR;
  motors_command(left_cps, right_cps);
}

void drive_stop(void) {
  rotate_cancel();
  motors_stop();
}
void drive_forward(float speed) {
  rotate_cancel();
  speed = fabsf(speed);
  drive_wheels(speed, speed);
}
void drive_backward(float speed) {
  rotate_cancel();
  speed = fabsf(speed);
  drive_wheels(-speed, -speed);
}
void drive_pivot_cw(float speed) {
  rotate_cancel();
  speed = fabsf(speed);
  drive_wheels(speed, -speed);
}
void drive_pivot_ccw(float speed) {
  rotate_cancel();
  speed = fabsf(speed);
  drive_wheels(-speed, speed);
}

// Compute speed for inner/outer wheel based on the difference
// in radius of their individual turns
//
// https://introtoroboticsv2.readthedocs.io/en/latest/course/driving/differential_steering.html
static void drive_arc(float speed, float radius_mm, bool right) {
  speed = fabsf(speed);
  radius_mm = fabsf(radius_mm);

  if (radius_mm < 1.0f) {
    right ? drive_pivot_cw(speed) : drive_pivot_ccw(speed);
    return;
  }

  float half = DRIVE_TRACK_WIDTH_MM / 2.0f;
  float outer = speed * (radius_mm + half) / radius_mm;
  float inner = speed * (radius_mm - half) / radius_mm;

  if (right)
    drive_wheels(outer, inner);
  else
    drive_wheels(inner, outer);
}

void drive_turn_right(float radius_mm) {
  rotate_cancel();
  drive_arc(DRIVE_TURN_SPEED, radius_mm, true);
}
void drive_turn_left(float radius_mm) {
  rotate_cancel();
  drive_arc(DRIVE_TURN_SPEED, radius_mm, false);
}

// measurements 210 desired 221 measured
// desired 120 got 130:

// ----------------NEW DEGREE ROTATION FUNCS---------------------------

// Yeah, these need to be tested
// The goal is to grab enc counts and measure how much the
// robot has turned by taking the current count - the start
//
// When the target is hit, stop the motor so the other can catch up
void drive_rotate_start(float degrees, float speed) {
  s_rot.startL = left_encoder_count();
  s_rot.startR = right_encoder_count();
  s_rot.target = (int32_t)lroundf(fabsf(degrees) * DRIVE_COUNTS_PER_DEG);
  s_rot.speed = copysignf(fabsf(speed), degrees);
  s_rot.active = s_rot.target > 0;

  if (s_rot.active)
    drive_rotate_update();
  else
    motors_stop();
}

// Returns true if turn is finished, else false
bool drive_rotate_update(void) {
  if (!s_rot.active)
    return true;

  const int32_t doneL = abs(left_encoder_count() - s_rot.startL);
  const int32_t doneR = abs(right_encoder_count() - s_rot.startR);
  const bool leftDone = doneL >= s_rot.target;
  const bool rightDone = doneR >= s_rot.target;

  if (leftDone && rightDone) {
    motors_stop();
    s_rot.active = false;
    return true;
  }

  drive_wheels(leftDone ? 0.0f : s_rot.speed, rightDone ? 0.0f : -s_rot.speed);
  return false;
}
