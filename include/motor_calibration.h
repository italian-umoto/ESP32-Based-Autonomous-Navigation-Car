#pragma once

// Open-loop motor / encoder test helpers (see motor_calibration.cpp)
void run_motor_test();

// Feedforward: signed encoder counts per second -> signed duty ticks,
// interpolated from the measured calibration tables
float feedforward_left_speed(float cps);
float feedforward_right_speed(float cps);
