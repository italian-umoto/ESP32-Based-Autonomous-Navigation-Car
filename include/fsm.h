#pragma once
#include <stdint.h>
#include "color_sensor.h"

class FSM {
public:
    enum class BotRole { Bot1, Bot2 };
    enum class State {
        Idle, CrossOutbound, TurnBack, FindPrimaryLane, AlignWithPrimaryLane,
        FollowPrimaryLane, TurnTowardYellow, FindYellowLane, AlignWithYellowLane,
        FollowYellowLane, TurnTowardHome, CrossHome, Complete
    };
    void tick();

private:
    void transitionTo(State next);
    bool turn(int degrees); // positive = clockwise, negative = counterclockwise.
    void followLane(DetectedColor target, DetectedColor observed);

    // TODO default state is crossoutbound and solo true just for testing
    State state = State::CrossOutbound;
    BotRole role = BotRole::Bot1;
    bool solo = true;
    uint32_t manualCommand = 0;
    uint32_t manualValue = 0;

    int entryLeft = 0;
    int entryRight = 0;
    int primaryLaneMatches = 0;

};
