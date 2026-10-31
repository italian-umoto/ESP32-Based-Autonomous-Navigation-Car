#pragma once

constexpr float DRIVE_TRACK_WIDTH_MM = 150.0f;
constexpr float DRIVE_MAX_WHEEL_CPS = 1300.0f;

void drive_wheels(float left_cps, float right_cps);

/* drive_stop / forward / backward / pivot_cw / pivot_ccw
 *
 * Does what it says on the can
 *
 * Param: speed - cps for each wheel (sign is ignored)
 */
void drive_stop(void);
void drive_forward(float speed);
void drive_backward(float speed);
void drive_pivot_cw(float speed);
void drive_pivot_ccw(float speed);

/* drive_turn_right / drive_turn_left
 *
 * Drive along an arc at ARC_TURN_CPS (see drive.cpp)
 *
 * Param: radius_mm - radius of the arc. Under 1 mm pivots in place
 */
void drive_turn_right(float radius_mm);
void drive_turn_left(float radius_mm);

/* drive_turn
 *
 * Turn in place by a number of degrees, measured with the encoders.
 * Call once per tick until it returns true
 *
 * Params: degrees    - how far to turn (positive -> clockwise)
 *         startLeft  - left encoder count when the turn started
 *         startRight - right encoder count when the turn started
 *
 * Return: true -and stops- once both wheels have turned far enough
 */
bool drive_turn(int degrees, int startLeft, int startRight);
