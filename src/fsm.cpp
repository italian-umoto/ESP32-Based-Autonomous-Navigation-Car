#include <Arduino.h>
#include <stdlib.h>
#include "fsm.h"
#include "websocket.h"
#include "collision.h"
#include "drive.h"
#include "motors.h"

static void performLaneFollowing(DetectedColor target_color);
static constexpr int PRIMARY_LANE_CONFIRM_READINGS = 1;

// encoder counts per wheel for a 90-degree empirically
static constexpr int COUNTS_PER_90 = 850;
static constexpr float FORWARD_CPS = 300;
static constexpr float TURN_CPS = 600;

void FSM::tick() {
    Command cmd;
    const bool hasCommand = getCommand(cmd);

    // 0 --> stop
    if (hasCommand && cmd.value == 0) {
        solo = false;
        manualCommand = manualValue = 0;
        transitionTo(State::Idle);
    // (7,1) or (7,2) -> solo
    } else if (hasCommand && cmd.value == 7 && (cmd.altValue == 1 || cmd.altValue == 2)) {
        role = cmd.altValue == 1 ? BotRole::Bot1 : BotRole::Bot2;
        solo = true;
        manualCommand = manualValue = 0;
        transitionTo(State::CrossOutbound);
    // manual motions
    } else if (hasCommand && !solo && cmd.value < 7) {
        manualCommand = cmd.value;
        manualValue = cmd.altValue;
    }

    // read wall and color and switch states based on it
    const CollisionReading collision = readCollisionDetection();
    const bool wall = collision.imminent_collision;
    // if (wall) drive_stop();
    const colorReading reading = single_read();
    const DetectedColor color = classifyColor(reading);
    printReading(reading, color);
    delay(100);


    // manual commands
    if (!solo) {
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
        return;
    }

    // turn direction and color depend on bot. This reduces the number states, but we could also do explicit states for everything
    // Bot1 turns CCW and Bot2 turns CW always
    const DetectedColor primaryColor = role == BotRole::Bot1 ? DetectedColor::Blue : DetectedColor::Red;
    const int turnDirection = role == BotRole::Bot1 ? -90 : 90;

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
            // wait till we turn 180
            if (!turn(180)) break;
            transitionTo(State::FindPrimaryLane);
            break;
        case State::FindPrimaryLane:
            // require consequetive readings
            primaryLaneMatches = color == primaryColor ? primaryLaneMatches + 1 : 0;
            if (primaryLaneMatches >= PRIMARY_LANE_CONFIRM_READINGS) transitionTo(State::AlignWithPrimaryLane);
            else drive_forward(FORWARD_CPS);
            break;
        case State::AlignWithPrimaryLane:
            if (!turn(turnDirection)) break;
            transitionTo(State::FollowPrimaryLane);
            break;
        case State::FollowPrimaryLane:
            if (wall) transitionTo(State::TurnTowardYellow);
            else followLane(primaryColor, color);
            break;
        case State::TurnTowardYellow:
            if (!turn(turnDirection)) break;
            transitionTo(State::FindYellowLane);
            break;
        case State::FindYellowLane:
            if (wall) drive_stop();
            else if (color == DetectedColor::Yellow) transitionTo(State::AlignWithYellowLane);
            else drive_forward(FORWARD_CPS);
            break;
        case State::AlignWithYellowLane:
            if (!turn(turnDirection)) break;
            transitionTo(State::FollowYellowLane);
            break;
        case State::FollowYellowLane:
            if (wall) transitionTo(State::TurnTowardHome);
            else followLane(DetectedColor::Yellow, color);
            break;
        case State::TurnTowardHome:
            if (!turn(turnDirection)) break;
            transitionTo(State::CrossHome);
            break;
        case State::CrossHome:
            if (wall) transitionTo(State::Complete);
            else drive_forward(FORWARD_CPS);
            break;
    }
}

void FSM::transitionTo(State next) {
    drive_stop();
    state = next;
    entryLeft = left_encoder_count();
    entryRight = right_encoder_count();
    primaryLaneMatches = 0;

}

bool FSM::turn(int degrees) {
    int target = COUNTS_PER_90 * abs(degrees) / 90;
    int left = abs(left_encoder_count() - entryLeft);
    int right = abs(right_encoder_count() - entryRight);
    if (left >= target && right >= target) {
        drive_stop();
        return true;
    }
    // stop each wheel at its target while the other finishes the pivot.
    const float leftSpeed = left >= target ? 0 : (degrees > 0 ? TURN_CPS : -TURN_CPS);
    const float rightSpeed = right >= target ? 0 : (degrees > 0 ? -TURN_CPS : TURN_CPS);
    drive_wheels(leftSpeed, rightSpeed);
    return false;
}

void FSM::followLane(DetectedColor target, DetectedColor observed) {
    // Bot1 searches right for target lane, turns left after finding it to track the edge. vice versa for bot2
    const bool steerRight = role == BotRole::Bot1 ? observed != target : observed == target;
    if (steerRight) drive_wheels(200, 100);
    else drive_wheels(100, 200);
}

// unused atm
static void performLaneFollowing(DetectedColor target_color) {
    static int detected_in_phase_count = 0;
    static bool on_phase = false;
    static long swap_phase_millis = 0;

    const long swap_period = 300;
    const int slow_speed = 100;
    const int fast_speed = 100;

    const colorReading reading = single_read();
    const DetectedColor color = classifyColor(reading);
    printReading(reading, color);

    long now = millis();
    bool detected_in_phase = (detected_in_phase_count > 1) && on_phase || (detected_in_phase_count > 3) && (!on_phase);

    if (swap_phase_millis < now && detected_in_phase) {
        on_phase = !on_phase;
        swap_phase_millis = now + swap_period;
        detected_in_phase_count = 0;
        Serial.println("swap");
    }

    if (!detected_in_phase) {
        bool on_color = (target_color == color);
        if (on_phase) {
            // turn right until on color
            drive_wheels(fast_speed, slow_speed);


            if (on_color) {
                Serial.print("on count ");
                Serial.println(detected_in_phase_count);

                detected_in_phase_count ++;
            }
        } else {
            // turn left until off color
            drive_wheels(slow_speed, fast_speed);


            if (!on_color) {
                Serial.print("off count ");
                Serial.println(detected_in_phase_count);

                detected_in_phase_count ++;
            }
        }
    } else {
        drive_wheels(fast_speed, fast_speed);
    }
}
