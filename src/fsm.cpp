#include <Arduino.h>
#include "fsm.h"
#include "websocket.h"
#include "collision.h"
#include "drive.h"
#include "motors.h"
#include "lane_following.h"

static constexpr float FORWARD_CPS = 500;

void FSM::handleCommand() {
    Command cmd;
    const bool hasCommand = getCommand(cmd);

    // 0 -> stop
    if (hasCommand && cmd.value == 0) {
        solo = false;
        manualCommand = manualValue = 0;
        transitionTo(State::Idle);

    // 1-6 -> manual commands
    } else if (hasCommand && !solo && cmd.value < 7) {
        manualCommand = cmd.value;
        manualValue = cmd.altValue;

    // 7 -> solo
    } else if (hasCommand && cmd.value == 7 && (cmd.altValue == 1 || cmd.altValue == 2)) {
        role = cmd.altValue == 1 ? BotRole::Bot1 : BotRole::Bot2;
        primaryColor = role == BotRole::Bot1 ? DetectedColor::Blue : DetectedColor::Red;
        turnDirection = role == BotRole::Bot1 ? -90 : 90; // Bot1 turns CCW; Bot2 turns CW.
        solo = true;
        manualCommand = manualValue = 0;
        transitionTo(State::CrossOutbound);
    }
}

// store readings for this tick's collision and color checks
void FSM::readSensors() {
    const CollisionReading collision = readCollisionDetection();
    wall = collision.imminent_collision;
    const colorReading reading = single_read();
    color = classifyColor(reading);
    printReading(reading, color);
}

void FSM::handleManual() {
    if (wall) manualCommand = manualValue = 0;
    switch (manualCommand) {
        case 1: drive_forward(manualValue); break;
        case 2: drive_backward(manualValue); break;
        case 3: drive_pivot_cw(manualValue); break;
        case 4: drive_pivot_ccw(manualValue); break;
        case 5: drive_turn_right(manualValue); break;
        case 6: drive_turn_left(manualValue); break;
        default: drive_stop(); break;
    }
}

void FSM::transitionTo(State next) {
    drive_stop();
    state = next;
    entryLeft = left_encoder_count();
    entryRight = right_encoder_count();
    primaryLaneMatches = 0;
}

void FSM::tick() {
    handleCommand();
    readSensors();
    if (!solo) {handleManual(); return;}

    switch (state) {
        case State::Idle:
        case State::Complete:
            drive_stop();
            break;
        case State::CrossOutbound:
            if (wall) transitionTo(State::TurnBack);
            else drive_forward(FORWARD_CPS);
            break;
        case State::TurnBack:
            if (!drive_turn(180, entryLeft, entryRight)) break;
            transitionTo(State::FindPrimaryLane);
            break;
        case State::FindPrimaryLane:
            // require consequetive readings
            primaryLaneMatches = color == primaryColor ? primaryLaneMatches + 1 : 0;
            if (primaryLaneMatches >= 2) transitionTo(State::AlignWithPrimaryLane);
            else drive_forward(FORWARD_CPS);
            break;
        case State::AlignWithPrimaryLane:
            if (!drive_turn(turnDirection, entryLeft, entryRight)) break;
            transitionTo(State::FollowPrimaryLane);
            break;
        case State::FollowPrimaryLane:
            if (wall) transitionTo(State::TurnTowardYellow);
            else performLaneFollowing(primaryColor);
            break;
        case State::TurnTowardYellow:
            if (!drive_turn(turnDirection, entryLeft, entryRight)) break;
            transitionTo(State::FindYellowLane);
            break;
        case State::FindYellowLane:
            if (wall) drive_stop();
            else if (color == DetectedColor::Yellow) transitionTo(State::AlignWithYellowLane);
            else drive_forward(FORWARD_CPS);
            break;
        case State::AlignWithYellowLane:
            if (!drive_turn(turnDirection, entryLeft, entryRight)) break;
            transitionTo(State::FollowYellowLane);
            break;
        case State::FollowYellowLane:
            if (wall) transitionTo(State::TurnTowardHome);
            else performLaneFollowing(DetectedColor::Yellow);
            break;
        case State::TurnTowardHome:
            if (!drive_turn(turnDirection, entryLeft, entryRight)) break;
            transitionTo(State::CrossHome);
            break;
        case State::CrossHome:
            if (wall) transitionTo(State::Complete);
            else drive_forward(FORWARD_CPS);
            break;
    }
}