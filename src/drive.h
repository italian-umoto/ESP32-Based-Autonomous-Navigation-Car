// Units:
//   speed  : encoder counts per second
//   radius : mm, from the turn center to the middle of the robot.
#pragma once

// Distance between the wheels
#define DRIVE_TRACK_WIDTH_MM   150.0f

// Highest cps the motors can reliably hold under load. (This is conservative)
// This was taking 75% of what the encoder count was at BDC_MCPWM_DUTY_TICK_MAX - 1
#define DRIVE_MAX_WHEEL_CPS    1300.0f


/* drive_stop/forward/backward/pivot cw,ccw/turn left,right
 *
 * Does what it says on the can
 *
 * Params: speed     - speed (in cps) for both motors on forward/backward
 *                     speed of robot on an arc for pivots/turns
 *
 *         radius_mm - the radius of the turn arc
 *
 * Effects: Will cause robot to move (or stop moving)
 */
void drive_stop(void);
void drive_forward(float speed);
void drive_backward(float speed);
void drive_pivot_cw(float speed);
void drive_pivot_ccw(float speed);
void drive_turn_right(float radius_mm);
void drive_turn_left(float radius_mm);
