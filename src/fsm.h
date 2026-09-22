#pragma once
#include <stdint.h>

class FSM {
    public:
        void tick();
        FSM();

    private:
        void stateIdle();
        void stateOne();
        void stateTwo();
        void stateThree();
        void stateFour();
        void stateFive();
        void stateSix();

        void checkCommandUpdate();
        void setState(int32_t stateNum);
        void setMotor(int32_t state, int32_t value);

        void (FSM::*currentState)() = &FSM::stateIdle;

        static constexpr void (FSM::*statesList[])() = {
            &FSM::stateIdle,
            &FSM::stateOne,
            &FSM::stateTwo,
            &FSM::stateThree,
            &FSM::stateFour,
            &FSM::stateFive,
            &FSM::stateSix,
        };

        // 7 is set in canvas assignment
        uint32_t state = 0;
        static constexpr int numStates = 7;
};
