#include <math.h>
#include "drive.h"
#include "motors.h"

#define LEFT_WHEEL_DIR   -1
#define RIGHT_WHEEL_DIR  1

#define DRIVE_TURN_SPEED 200

// Scale down the inputs to clamp in the max CPS
static void drive_wheels(float left_cps, float right_cps)
{
    float peak = fmaxf(fabsf(left_cps), fabsf(right_cps));
    if (peak > DRIVE_MAX_WHEEL_CPS) {
        float k = DRIVE_MAX_WHEEL_CPS / peak;
        left_cps  *= k;
        right_cps *= k;
    }

    left_cps  *= LEFT_WHEEL_DIR;
    right_cps *= RIGHT_WHEEL_DIR;
    motors_command(left_cps, right_cps);
}

void drive_stop(void)             { motors_stop(); }
void drive_forward(float speed)   { speed = fabsf(speed); drive_wheels( speed,  speed); }
void drive_backward(float speed)  { speed = fabsf(speed); drive_wheels(-speed, -speed); }
void drive_pivot_cw(float speed)  { speed = fabsf(speed); drive_wheels( speed, -speed); }
void drive_pivot_ccw(float speed) { speed = fabsf(speed); drive_wheels(-speed,  speed); }


// Compute speed for inner/outer wheel based on the difference 
// in radius of their individual turns
//
// https://introtoroboticsv2.readthedocs.io/en/latest/course/driving/differential_steering.html
static void drive_arc(float speed, float radius_mm, bool right)
{
    speed     = fabsf(speed);
    radius_mm = fabsf(radius_mm);

    if (radius_mm < 1.0f) {
        right ? drive_pivot_cw(speed) : drive_pivot_ccw(speed);
        return;
    }

    float half  = DRIVE_TRACK_WIDTH_MM / 2.0f;
    float outer = speed * (radius_mm + half) / radius_mm;
    float inner = speed * (radius_mm - half) / radius_mm;

    if (right) drive_wheels(outer, inner);
    else       drive_wheels(inner, outer);
}

void drive_turn_right(float radius_mm) { drive_arc(DRIVE_TURN_SPEED, radius_mm, true);  }
void drive_turn_left (float radius_mm) { drive_arc(DRIVE_TURN_SPEED, radius_mm, false); }
