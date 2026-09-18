#include "fsm.h"
#include <stdint.h>
//#include "websocket.h"


#define STATE_TRANSITION_PIN 5
#define DEBOUNCE_DELAY 10


// I'll be honest I ripped this straight off of google
bool buttonPressed() {
  static int lastButtonState = HIGH;
  static int currentButtonState = HIGH;
  static unsigned long lastDebounceTime = 0;

  int reading = digitalRead(STATE_TRANSITION_PIN);

  if (reading != lastButtonState) {
    lastDebounceTime = millis();
    lastButtonState = reading;
  }

  if ((millis() - lastDebounceTime) > DEBOUNCE_DELAY) {
    if (reading != currentButtonState) {
      currentButtonState = reading;

      if (currentButtonState == LOW) {
        return true; 
      }
    }
  }
  
  return false;
}

FSM::FSM() {
  pinMode(STATE_TRANSITION_PIN, INPUT_PULLUP);
}

void FSM::tick() {
    (this->*currentState)();
}

void FSM::setState(int32_t stateNum) {
    if (stateNum < 0 || stateNum >= numStates) {
        Serial.print("Invalid state: ");
        Serial.println(stateNum);
        return;
    }
    this->state = stateNum;
    currentState = statesList[stateNum];
}

void FSM::checkChangeState() {
    Command cmd;
    if (get_command(cmd) && cmd.type == CommandType::SET_STATE) {
        setState(cmd.value);
    } else if (buttonPressed()) {
        setState(this->state + 1);
    }
} 




/*
 * the reason I put all of the states as separate functions
 * instead of just using an enum is so that we can 
 * very easily move from the milestone to actually doing
 * things for each state, so now we could just fill in
 * whatever state with the behavior it needs
 */
void FSM::stateIdle() {
    checkChangeState();
    Serial.println("In Idle state :)");
}

void FSM::stateOne() {
    checkChangeState();
    Serial.println("In state one");
}

void FSM::stateTwo() {
    checkChangeState();
    Serial.println("In state two");
}

void FSM::stateThree() {
    checkChangeState();
    Serial.println("In state three");
}

void FSM::stateFour() {
    checkChangeState();
    Serial.println("In state four");
}

void FSM::stateFive() {
    checkChangeState();
    Serial.println("In state five");
}

void FSM::stateSix() {
    checkChangeState();
    Serial.println("In state six");
}




