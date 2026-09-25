#pragma once
#include <stdint.h>

/* Our sick and cool state machine class
 *
 * Each state is associated with a movement (see states below)
 * States can change via websocket command or on-breadboard button press
 */
class FSM {
    public:
        /* tick
         * calls state method according to this->currentState
         */
        void tick();

        /* FSM
         * sets pin for the fun rgbled state identifier
         */
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

        // Current state
        // 0: Stop 
        // 1: Forward 
        // 2: Backward 
        // 3: Pivot CW 
        // 4: Pivot CCW 
        // 5: Right Turn 
        // 6: Left Turn
        uint32_t state = 0;

        // Value given after comma in websocket command 
        // (either speed or radius dependent on state)
        uint32_t cmd_value = 0;
};
