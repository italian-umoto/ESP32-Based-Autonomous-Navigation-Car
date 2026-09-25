/* motors.h
 *
 * Direction Convention:
 *  Positive in the motor module means the direction a positive duty 
 *  spins that motor. Left motor is mirrored on the robot,
 *  so positive left command => backwards physical motion.
 *  This is accounted for in drive module
 */

#pragma once
#include <stdint.h>
#include "esp_err.h"


// ===================================================================
// PWM config
// ===================================================================

// Tick rate of the MCPWM counter
#define BDC_MCPWM_TIMER_RESOLUTION_HZ 10000000

// H-bridge PWM frequency (datasheet limit is 5 kHz, choosing 2k for safety)
#define BDC_MCPWM_FREQ_HZ             2000

// Ticks per PWM period (5000 at 2 kHz) 
// Duty ranges 0 .. BDC_MCPWM_DUTY_TICK_MAX - 1
// Pin is high while counter < duty
#define BDC_MCPWM_DUTY_TICK_MAX       (BDC_MCPWM_TIMER_RESOLUTION_HZ / BDC_MCPWM_FREQ_HZ)


// ===================================================================
// Types / globals
// ===================================================================

/* bdc_motor_t
 *
 * Opaque: the MCPWM handles live in motors.cpp, callers only pass pointers
 */
typedef struct bdc_motor_t bdc_motor_t;

/* left_motor, right_motor
 *
 * Effects: Exposed for debugging only - overwritten by PID
 *          Use motors_command() / motors_stop()
 */
extern bdc_motor_t left_motor;
extern bdc_motor_t right_motor;


// ===================================================================
// Setup
// ===================================================================

/* setup_motors
 *
 * Create both motors and encoders, zero encoder counts, start the PID timer 
 * Motors start braked 
 * Call once from setup() 
 * 
 * Effects: Claims 2 MCPWM groups, 2 PCNT units, and one esp_timer
 *          Aborts (ESP_ERROR_CHECK) if any peripheral fails to initialize
 */
void setup_motors();


// ===================================================================
// Command interface
// ===================================================================

/* motors_command
 *
 * Set a target speed for each wheel
 * The PID loop picks it up on its next tick
 *
 * Param:  left_cps  - left wheel target, encoder counts per second
 *         right_cps - right wheel target, encoder counts per second
 *
 * Effects: Enables the PID if it was stopped on next tick
 */
void motors_command(float left_cps, float right_cps);

/* motors_stop
 *
 * Disable the PID and brake both motors
 * Applied on the next PID tick
 */
void motors_stop(void);


// ===================================================================
// Encoders
// ===================================================================

/* left_encoder_count, right_encoder_count
 *
 * Return: Accumulated count, signed, continuous across the +/-1000 hardware limits
 *         Zeroed at setup_motors(). Positive = motor-channel positive
 *         direction
 *         Will take 10 days+ to wrap
 *
 * Effects: Aborts if called before setup_motors() (PCNT handle is NULL)
 */
int left_encoder_count();
int right_encoder_count();


// ===================================================================
// Module-Specific functions
// ===================================================================

/* left_set_signed_speed, right_set_signed_speed
 *
 * Drive a motor at a fixed duty
 *
 * Param:  speed - sign picks direction (> 0 forward, <= 0 reverse),
 *                 magnitude is duty ticks clamped
 *
 * Effects: Overwritten within 50 ms if the PID is enabled
 */
void left_set_signed_speed(int speed);
void right_set_signed_speed(int speed);

/* bdc_motor_set_speed
 *
 * Param:  speed - PWM on-time in ticks, 0 .. BDC_MCPWM_DUTY_TICK_MAX - 1
 * Return: ESP_OK, or the MCPWM error
 */
esp_err_t bdc_motor_set_speed(bdc_motor_t *motor, uint32_t speed);

/* bdc_motor_forward / reverse / coast / brake
 *
 * forward - PWM on channel A, B held low
 * reverse - PWM on channel B, A held low
 * coast   - both low, motor spins freely
 * brake   - both high, windings shorted, motor stops quickly
 *
 * Return: ESP_OK, or the MCPWM error
 * Effects: Overridden by the PID while it is enabled
 */
esp_err_t bdc_motor_forward(bdc_motor_t *motor);
esp_err_t bdc_motor_reverse(bdc_motor_t *motor);
esp_err_t bdc_motor_coast(bdc_motor_t *motor);
esp_err_t bdc_motor_brake(bdc_motor_t *motor);

/* print_pid_debug
 *E (328) esp_core_dump_flash: No core dump partition found!
Max-cps test in 3 s...
L: -1063 -> -2812  (1749 cps)
R: 2394 -> 4387  (1993 cps)
......
Wi-Fi connected
 * Print one line per wheel: enabled, setpoint, measured cps, error,
 * PID output, encoder count
 *
 * Effects: Reads the encoders -- must be called after setup_motors()
 */
void print_pid_debug();



























