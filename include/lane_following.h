#pragma once
#include "color_sensor.h"

enum class LaneStatus {
    Following,
    Lost,
};

LaneStatus followLane(DetectedColor color);
void stopLaneFollowing();
