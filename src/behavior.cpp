#include "behavior.h"
#include "drive.h"
#include "lane_following.h"
#include <Arduino.h>

constexpr float ROTATE_CPS = 600.0f; // TUNABLE

void Behavior::start(Motion motion, float param, DetectedColor color) {
  exitCurrent();

  m_motion = motion;

  switch (motion) {
  case Motion::Stop:
    drive_stop();
    break;
  case Motion::Forward:
    drive_forward(param);
    break;
  case Motion::Backward:
    drive_backward(param);
    break;
  case Motion::PivotCW:
    drive_pivot_cw(param);
    break;
  case Motion::PivotCCW:
    drive_pivot_ccw(param);
    break;
  case Motion::TurnRight:
    drive_turn_right(param);
    break;
  case Motion::TurnLeft:
    drive_turn_left(param);
    break;
  case Motion::Rotate:
    drive_rotate_start(param, ROTATE_CPS);
    break;
  case Motion::FollowLane:
    laneStart(color);
    break;
  }
}

BehaviorStatus Behavior::tick() {
  switch (m_motion) {
  case Motion::Stop:
    return BehaviorStatus::Done;
  case Motion::Rotate:
    return drive_rotate_update() ? BehaviorStatus::Done
                                 : BehaviorStatus::Running;
  case Motion::FollowLane:
    laneStep();
    return BehaviorStatus::Running;
  default:
    return BehaviorStatus::Running;
  }
}

bool Behavior::movesForward() const {
  switch (m_motion) {
  case Motion::Forward:
  case Motion::TurnLeft:
  case Motion::TurnRight:
  case Motion::FollowLane:
    return true;
  default:
    return false;
  }
}

// Clean up whatever the current motion left running
// Is only called in Behavior::start()
void Behavior::exitCurrent() {
  if (m_motion == Motion::FollowLane)
    laneStop();
}
