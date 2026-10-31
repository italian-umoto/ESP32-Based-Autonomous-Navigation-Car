#pragma once
#include "color_sensor.h"
#include <stdint.h>

class FSM {
public:
  enum class State {
    Idle,
    CrossOutbound,
    TurnBack,
    FindPrimaryLane,
    AdvancePrimaryLane,
    AlignWithPrimaryLane,
    FollowPrimaryLane,
    TurnTowardYellow,
    FindYellowLane,
    AdvanceYellowLane,
    AlignWithYellowLane,
    FollowYellowLane,
    TurnTowardHome,
    CrossHome,
    Complete
  };

  void begin();
  void tick();

private:
  void handleCommand();
  void readSensors();
  void handleManual();
  void runRoute();

  // used to handle movement between steps
  void startRoute();
  void transitionTo(State next);

  // implementation of "state logic"
  void driveUntilWall(State next);
  void turnThen(int degrees, State next);
  void searchForLane(DetectedColor color, State next, bool stopAtWall);
  void advanceThen(State next);
  void followLaneUntilWall(DetectedColor color, State next);

  // helpers
  bool buttonPressed();
  void showState();

  // mode and state
  bool solo = false;
  State state = State::Idle;
  uint32_t manualCommand = 0;
  uint32_t manualValue = 0;

  // this tick's sensor readings
  bool wall = false;
  ColorReadings readings{};
  DetectedColor leftColor = DetectedColor::Dark;
  DetectedColor rightColor = DetectedColor::Dark;

  // recorded when the current state started
  uint32_t stateEntryMs = 0;
  int entryLeft = 0;   // left encoder count
  int entryRight = 0;  // right encoder count
  int laneMatches = 0; // consecutive ticks the target lane color was seen
};
