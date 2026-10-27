#include "route.h"
#include "bot_config.h"
#include "color_sensor.h"
#include <Arduino.h>

constexpr float CROSS_CPS = 500.0f; // TUNABLE: floor crossing speed
constexpr float FIND_CPS = 400.0f;  // TUNABLE: robot speed when finding color

constexpr uint8_t CONFIRM_READS = 4; // TUNABLE: consecutive hits for Wall/Color

constexpr DetectedColor NONE = DetectedColor::Dark;
constexpr DetectedColor YELLOW = DetectedColor::Yellow;

// ------------------------------------------------------------------
// ROUTE is hardcoded here
//
// bot_config.h supplies:
//   PRIMARY  - red for bot 1, blue for bot 2
//   TURN_DEG - -90 for bot 1 (left), +90 for bot 2 (right)
// ------------------------------------------------------------------
constexpr Step ROUTE[] = {
    // name        motion          param      color  exitCondition
    {"cross out", Motion::Forward, CROSS_CPS, NONE, Exit::Wall},
    {"turn back", Motion::Rotate, 2 * TURN_DEG, NONE, Exit::Done},
    {"find primary", Motion::Forward, FIND_CPS, PRIMARY, Exit::Color},
    {"align primary", Motion::Rotate, TURN_DEG, NONE, Exit::Done},
    {"follow primary", Motion::FollowLane, 0, PRIMARY, Exit::Wall},
    {"turn to yellow", Motion::Rotate, TURN_DEG, NONE, Exit::Done},
    {"find yellow", Motion::Forward, FIND_CPS, YELLOW, Exit::Color},
    {"align yellow", Motion::Rotate, TURN_DEG, NONE, Exit::Done},
    {"follow yellow", Motion::FollowLane, 0, YELLOW, Exit::Wall},
    {"turn home", Motion::Rotate, TURN_DEG, NONE, Exit::Done},
    {"cross home", Motion::Forward, CROSS_CPS, NONE, Exit::Wall},
    {"complete", Motion::Stop, 0, NONE, Exit::Never},
};

constexpr uint8_t ROUTE_LEN = sizeof(ROUTE) / sizeof(ROUTE[0]);

const Step &Route::start(uint8_t fromStep) {
  m_idx = fromStep < ROUTE_LEN ? fromStep : ROUTE_LEN - 1;
  m_confirm = 0;
  return ROUTE[m_idx];
}

const Step &Route::current() const { return ROUTE[m_idx]; }

const Step *Route::tick(bool wall, BehaviorStatus status) {
  if (!exitMet(ROUTE[m_idx], wall, status))
    return nullptr;

  // Exit::Never on the last step means we never get here past the end,
  // but guard anyway
  if (m_idx + 1 >= ROUTE_LEN)
    return nullptr;

  ++m_idx;
  m_confirm = 0;
  return &ROUTE[m_idx];
}

bool Route::exitMet(const Step &step, bool wall, BehaviorStatus status) {
  // Require CONFIRM_READS consecutive hits for noise on sensors
  auto confirmed = [this](bool hit) {
    m_confirm = hit ? m_confirm + 1 : 0;
    return m_confirm >= CONFIRM_READS;
  };

  switch (step.exit) {
  case Exit::Never: // fin
    return false;
  case Exit::Done: // happens for turns
    return status == BehaviorStatus::Done;
  case Exit::Wall: // when we want to hit a wall
    return confirmed(wall);
  case Exit::Color: // looking for a color
    return confirmed(bothSensorsSee(step.color));
  }
  return false;
}
