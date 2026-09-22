#pragma once
#include <stdint.h>
#include "esp_err.h"

class FSM {
    public:
        void tick();
        FSM();

    private:
        void stateStop();
        void stateForward();
        void stateBackward();
        void statePivotCW();
        void statePivotCCW();
        void stateRightTurn();
        void stateLeftTurn();

        void checkCommandUpdate();
        void updateState(int32_t state, int32_t value);

        void (FSM::*currentState)() = &FSM::stateStop;

        static constexpr void (FSM::*statesList[])() = {
            &FSM::stateStop,
            &FSM::stateForward,
            &FSM::stateBackward,
            &FSM::statePivotCW,
            &FSM::statePivotCCW,
            &FSM::stateRightTurn,
            &FSM::stateLeftTurn,
        };

        // 7 is set in canvas assignment
        uint32_t state = 0;
        static constexpr int numStates = 7;
};
