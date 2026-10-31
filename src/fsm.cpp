#include "fsm.h"
#include "bot_config.h"
#include "collision.h"
#include "drive.h"
#include "lane_following.h"
#include "motors.h"
#include "pins.h"
#include "websocket.h"
#include <Arduino.h>

static constexpr float FORWARD_CPS = 800;
static constexpr int CONFIRM_READS = 2;
// Forward duration before alignment. Starting estimate for about 3 inches;
// tune on the bot at FORWARD_CPS.
static constexpr uint32_t ALIGN_ADVANCE_MS = 1700;
static constexpr int TURNAROUND_DEG = 180;

// websocket commands / manualCommand values
static constexpr int CMD_STOP = 0;
static constexpr int CMD_FORWARD = 1;    // altValue = speed (cps)
static constexpr int CMD_BACKWARD = 2;   // altValue = speed (cps)
static constexpr int CMD_PIVOT_CW = 3;   // altValue = speed (cps)
static constexpr int CMD_PIVOT_CCW = 4;  // altValue = speed (cps)
static constexpr int CMD_TURN_RIGHT = 5; // altValue = radius (mm)
static constexpr int CMD_TURN_LEFT = 6;  // altValue = radius (mm)
static constexpr int CMD_START_ROUTE = 7;

// for button and onboard LED
static constexpr uint32_t BUTTON_DEBOUNCE_MS = 10;
static constexpr uint8_t BRIGHT = 64;

struct Rgb {
  uint8_t r, g, b;
};

// colors for steps
static constexpr Rgb MANUAL_COLOR = {BRIGHT / 4, BRIGHT / 4, BRIGHT / 4};
static constexpr Rgb ROUTE_COLORS[] = {
    {BRIGHT, 0, 0},      {0, BRIGHT, 0},      {0, 0, BRIGHT},
    {BRIGHT, BRIGHT, 0}, {BRIGHT, 0, BRIGHT}, {0, BRIGHT, BRIGHT},
};
static constexpr int NUM_ROUTE_COLORS =
    sizeof(ROUTE_COLORS) / sizeof(ROUTE_COLORS[0]);

// for debug
static const char *stateName(FSM::State state) {
  switch (state) {
  case FSM::State::Idle:
    return "Idle";
  case FSM::State::CrossOutbound:
    return "CrossOutbound";
  case FSM::State::TurnBack:
    return "TurnBack";
  case FSM::State::FindPrimaryLane:
    return "FindPrimaryLane";
  case FSM::State::AdvancePrimaryLane:
    return "AdvancePrimaryLane";
  case FSM::State::AlignWithPrimaryLane:
    return "AlignWithPrimaryLane";
  case FSM::State::FollowPrimaryLane:
    return "FollowPrimaryLane";
  case FSM::State::TurnTowardYellow:
    return "TurnTowardYellow";
  case FSM::State::FindYellowLane:
    return "FindYellowLane";
  case FSM::State::AdvanceYellowLane:
    return "AdvanceYellowLane";
  case FSM::State::AlignWithYellowLane:
    return "AlignWithYellowLane";
  case FSM::State::FollowYellowLane:
    return "FollowYellowLane";
  case FSM::State::TurnTowardHome:
    return "TurnTowardHome";
  case FSM::State::CrossHome:
    return "CrossHome";
  case FSM::State::Complete:
    return "Complete";
  }
  return "Unknown";
}

void FSM::begin() {
  pinMode(STATE_TRANSITION_PIN, INPUT_PULLUP);
  showState();
}

void FSM::tick() {
  handleCommand();
  readSensors();
  if (solo)
    runRoute();
  else
    handleManual();
}

void FSM::handleCommand() {
  // read the button
  const bool buttonWasPressed = buttonPressed();

  Command cmd;
  if (getCommand(cmd)) {
    if (cmd.value == CMD_STOP) {
      solo = false;
      manualCommand = manualValue = 0;
      transitionTo(State::Idle);
    } else if (cmd.value == CMD_START_ROUTE) {
      startRoute();
    } else if (!solo && cmd.value < CMD_START_ROUTE) {
      // ignores all other websocket commands for now in solo...
      manualCommand = cmd.value;
      manualValue = cmd.altValue;
    }
  } else if (buttonWasPressed) {
    startRoute();
  }
}

// store readings for this tick's collision and color checks
void FSM::readSensors() {
  const CollisionReading collision = readCollisionDetection();
  wall = collision.imminent_collision;

  readings = readColors();
  leftColor = classifyColor(readings.left);
  rightColor = classifyColor(readings.right);

  Serial.print("Left:  ");
  printReading(readings.left, leftColor);
  Serial.print("Right: ");
  printReading(readings.right, rightColor);
  Serial.printf("Collision reflected: %d, wall: %d\n", collision.reflected,
                wall);
}

void FSM::handleManual() {
  // a wall cancels whatever manual command was running
  if (wall)
    manualCommand = manualValue = 0;

  switch (manualCommand) {
  case CMD_FORWARD:
    drive_forward(manualValue);
    break;
  case CMD_BACKWARD:
    drive_backward(manualValue);
    break;
  case CMD_PIVOT_CW:
    drive_pivot_cw(manualValue);
    break;
  case CMD_PIVOT_CCW:
    drive_pivot_ccw(manualValue);
    break;
  case CMD_TURN_RIGHT:
    drive_turn_right(manualValue);
    break;
  case CMD_TURN_LEFT:
    drive_turn_left(manualValue);
    break;
  default:
    drive_stop();
    break;
  }
}

// do the step
void FSM::runRoute() {
  switch (state) {
  case State::Idle:
  case State::Complete:
    drive_stop();
    break;

  // ALL OF THESE ARE TAKING NEXT STATE AS A PARAM
  // go forward -> turn around -> find the primary lane
  case State::CrossOutbound:
    driveUntilWall(State::TurnBack);
    break;
  case State::TurnBack:
    turnThen(TURNAROUND_DEG, State::FindPrimaryLane);
    break;
  case State::FindPrimaryLane:
    searchForLane(PRIMARY, State::AdvancePrimaryLane, false);
    break;
  case State::AdvancePrimaryLane:
    advanceThen(State::AlignWithPrimaryLane);
    break;
  case State::AlignWithPrimaryLane:
    turnThen(TURN_DEG, State::FollowPrimaryLane);
    break;
  case State::FollowPrimaryLane:
    followLaneUntilWall(PRIMARY, State::TurnTowardYellow);
    break;

  // go to yellow lane
  case State::TurnTowardYellow:
    turnThen(TURN_DEG, State::FindYellowLane);
    break;
  case State::FindYellowLane:
    searchForLane(DetectedColor::Yellow, State::AdvanceYellowLane, true);
    break;
  case State::AdvanceYellowLane:
    advanceThen(State::AlignWithYellowLane);
    break;
  case State::AlignWithYellowLane:
    turnThen(TURN_DEG, State::FollowYellowLane);
    break;
  case State::FollowYellowLane:
    followLaneUntilWall(DetectedColor::Yellow, State::TurnTowardHome);
    break;

  // go home
  case State::TurnTowardHome:
    turnThen(TURN_DEG, State::CrossHome);
    break;
  case State::CrossHome:
    driveUntilWall(State::Complete);
    break;
  }
}

void FSM::startRoute() {
  solo = true;
  manualCommand = manualValue = 0;
  transitionTo(State::CrossOutbound);
}

void FSM::transitionTo(State next) {
  drive_stop();
  state = next;
  stateEntryMs = millis();
  entryLeft = left_encoder_count();
  entryRight = right_encoder_count();
  laneMatches = 0;

  Serial.printf("State -> %s\n", stateName(next));
  showState();
}

// -------------------------------------------------------------------
// STATE IMPLEMENTATIONS
// -------------------------------------------------------------------

void FSM::driveUntilWall(State next) {
  if (wall)
    transitionTo(next);
  else
    drive_forward(FORWARD_CPS);
}

void FSM::turnThen(int degrees, State next) {
  if (drive_turn(degrees, entryLeft, entryRight))
    transitionTo(next);
}

void FSM::searchForLane(DetectedColor color, State next, bool stopAtWall) {
  const bool seen = (leftColor == color || rightColor == color);
  laneMatches = seen ? laneMatches + 1 : 0;

  if (stopAtWall && wall)
    drive_stop();
  else if (laneMatches >= CONFIRM_READS)
    transitionTo(next);
  else
    drive_forward(FORWARD_CPS);
}

// this is to put the robot over the line it just found
void FSM::advanceThen(State next) {
  if (millis() - stateEntryMs < ALIGN_ADVANCE_MS)
    drive_forward(FORWARD_CPS);
  else
    transitionTo(next);
}

void FSM::followLaneUntilWall(DetectedColor color, State next) {
  if (wall)
    transitionTo(next);
  else
    performLaneFollowing(color, readings);
}

// -------------------------------------------------------------------
// helpers
// -------------------------------------------------------------------

bool FSM::buttonPressed() {
  static int lastReading = HIGH;
  static int stableState = HIGH;
  static uint32_t lastChangeMs = 0;

  const int reading = digitalRead(STATE_TRANSITION_PIN);
  const uint32_t now = millis();

  if (reading != lastReading) {
    lastChangeMs = now;
    lastReading = reading;
  }

  if (now - lastChangeMs > BUTTON_DEBOUNCE_MS && reading != stableState) {
    stableState = reading;
    return stableState == LOW;
  }
  return false;
}

void FSM::showState() {
  const Rgb c = solo ? ROUTE_COLORS[static_cast<int>(state) % NUM_ROUTE_COLORS]
                     : MANUAL_COLOR;
  rgbLedWrite(STATE_INDICATOR_LED, c.r, c.g, c.b);
}
