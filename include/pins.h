#pragma once

// motor and encoder
static constexpr int LEFT_M1 = 21;
static constexpr int LEFT_M2 = 4;
static constexpr int LEFT_A = 41;
static constexpr int LEFT_B = 42;
static constexpr int RIGHT_M1 = 39;
static constexpr int RIGHT_M2 = 40;
static constexpr int RIGHT_A = 2;
static constexpr int RIGHT_B = 1;

// color sensor and visible LEDs
static constexpr int COLOR_SENSOR_PIN = 16;
static constexpr int RED_LED_PIN = 6;
static constexpr int GREEN_LED_PIN = 7;
static constexpr int BLUE_LED_PIN = 15;

// collision detection
static constexpr int IR_LED_PIN = 8;
static constexpr int COLLISION_SENSOR_PIN = 9;

// state transitions
static constexpr int STATE_INDICATOR_LED = 38;
static constexpr int STATE_TRANSITION_PIN = 5;
