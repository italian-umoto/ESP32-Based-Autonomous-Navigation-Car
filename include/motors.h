#pragma once
#include <stdint.h>
#include "esp_err.h"

// How quickly the pwm counter counts
#define BDC_MCPWM_TIMER_RESOLUTION_HZ 10000000
// H-bridge pwm frequency (datasheet says limit is 5khz, choosing 2k for safety)
#define BDC_MCPWM_FREQ_HZ             2000
// 0 is minimum duty, BDC_MCPWM_DUTY_TICK_MAX - 1 is max duty. Pin is high when counter < speed
#define BDC_MCPWM_DUTY_TICK_MAX       (BDC_MCPWM_TIMER_RESOLUTION_HZ / BDC_MCPWM_FREQ_HZ)

// Opaque: the MCPWM handles live in motors.cpp, callers only pass pointers
typedef struct bdc_motor_t bdc_motor_t;

extern bdc_motor_t left_motor;
extern bdc_motor_t right_motor;

// Call setup_motors() once from setup(), loop_motors() from loop()
void setup_motors();
void loop_motors();

// speed is the PWM on-time in ticks, 0 .. BDC_MCPWM_DUTY_TICK_MAX - 1
esp_err_t bdc_motor_set_speed(bdc_motor_t *motor, uint32_t speed);

esp_err_t bdc_motor_forward(bdc_motor_t *motor);
esp_err_t bdc_motor_reverse(bdc_motor_t *motor);
esp_err_t bdc_motor_coast(bdc_motor_t *motor);
esp_err_t bdc_motor_brake(bdc_motor_t *motor);

// Raw accumulated quadrature count (4 counts per encoder cycle), signed 32-bit,
// continuous across the +/-1000 hardware limits. Zeroed at setup_motors().
// Forward is positive. Will take 10 days @ 100% to wrap (so it won't)
int left_encoder_count();
int right_encoder_count();

// Open-loop: sign picks direction, magnitude is duty ticks (0 .. BDC_MCPWM_DUTY_TICK_MAX - 1)
void left_set_signed_speed(int speed);
void right_set_signed_speed(int speed);

// Closed-loop. enable_pid() snapshots both encoder counts and turns on the
// loop for both sides; the set_speed_pid calls do this automatically if needed.
// Setpoints are in encoder counts per second, forward positive.
void enable_pid();
void left_set_speed_pid(int speed);
void right_set_speed_pid(int speed);
void print_pid_debug();
