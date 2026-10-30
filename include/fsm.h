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
    void handleCommand();
    void handleManual();
    void readSensors();
    void transitionTo(State next);

    // TODO change to idle and false later
    bool solo = true;
    State state = State::CrossOutbound;
    BotRole role = BotRole::Bot1;
    DetectedColor primaryColor = DetectedColor::Blue;
    int turnDirection = -90; 
    uint32_t manualCommand = 0;
    uint32_t manualValue = 0;

    bool wall = false;
    DetectedColor color = DetectedColor::Dark;

    int entryLeft = 0;
    int entryRight = 0;
    int primaryLaneMatches = 0;
};
