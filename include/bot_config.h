#pragma once
#include "color_sensor.h"

#define BOT_ID 1

#if BOT_ID == 1
constexpr DetectedColor PRIMARY = DetectedColor::Red;
constexpr int TURN_DEG = -90;
#elif BOT_ID == 2
constexpr DetectedColor PRIMARY = DetectedColor::Blue;
constexpr int TURN_DEG = +90;
#else
#error "BOT_ID must be 1 or 2"
#endif
