#pragma once
#include "color_sensor.h"

enum class LaneStatus {
  Following,
  Lost,
};

void laneStart(DetectedColor color);
LaneStatus laneStep(void);
void laneStop(void);
