#include <stdint.h>
#include <Arduino.h> 
#include "websocket.h"
#include "drive.h"
#include "fsm.h"

#define STATE_INDICATOR_LED 38
#define BRIGHT 64
#define STATE_TRANSITION_PIN 5
#define DEBOUNCE_DELAY 1
#define NUM_STATES 7


static bool buttonPressed() {
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

void FSM::updateState(int32_t state, int32_t value) {
    Serial.printf("updateState(%ld, %ld) from state %ld\n",
                  (long)state, (long)value, (long)this->state);
    if (state < 0 || state >= NUM_STATES) {
        Serial.print("Invalid state: ");
        Serial.println(state);
        return;
    }
    this->state = state;
    this->cmd_value = value;
    currentState = statesList[state];
}

void FSM::checkCommandUpdate() {
    Command cmd;
    if (getCommand(cmd)) {
        updateState(cmd.value, cmd.altValue);
    } else if (buttonPressed()) {
        updateState((this->state + 1) % 7, 0);
    }
} 


//                     PIN     RED    GREEN   BLUE
// rgbLedWrite(RGB_BUILTIN, BRIGHT,       0,     0);
void FSM::stateStop() {
    checkCommandUpdate();
    Serial.println("State: Stopped");
    rgbLedWrite(STATE_INDICATOR_LED, BRIGHT, 0, 0);
    drive_stop();
    delay(10);
}

void FSM::stateForward() {
    checkCommandUpdate();
    Serial.println("State: Forward");
    rgbLedWrite(STATE_INDICATOR_LED, 0, BRIGHT, 0);
    drive_forward(this->cmd_value);
    delay(10);
}

void FSM::stateBackward() {
    checkCommandUpdate();
    Serial.println("State: Backward");
    Serial.printf("Speed: %d\n", this->cmd_value);
    rgbLedWrite(STATE_INDICATOR_LED, 0, 0, BRIGHT); 
    drive_backward(this->cmd_value);
    delay(10);
}

void FSM::statePivotCW() {
    checkCommandUpdate();
    Serial.println("State: Pivot CW");
    Serial.printf("Speed: %d\n", this->cmd_value);
    rgbLedWrite(STATE_INDICATOR_LED, BRIGHT, BRIGHT, 0);
    drive_pivot_cw(this->cmd_value);
    delay(10);
}

void FSM::statePivotCCW() {
    checkCommandUpdate();
    Serial.println("State: Pivot CW");
    Serial.printf("Speed: %d\n", this->cmd_value);
    rgbLedWrite(STATE_INDICATOR_LED, BRIGHT, 0, BRIGHT);
    drive_pivot_ccw(this->cmd_value);
    delay(10);
}

void FSM::stateRightTurn() {
    checkCommandUpdate();
    Serial.println("State: Right Turn");
    Serial.printf("Radius: %d\n", this->cmd_value);
    rgbLedWrite(STATE_INDICATOR_LED, 0, BRIGHT, BRIGHT);
    drive_turn_right(this->cmd_value);
    delay(10);
}

void FSM::stateLeftTurn() {
    checkCommandUpdate();
    Serial.println("State: Left Turn");
    Serial.printf("Radius: %d\n", this->cmd_value);
    rgbLedWrite(STATE_INDICATOR_LED, BRIGHT, BRIGHT, BRIGHT);
    drive_turn_left(this->cmd_value);
    delay(10);
}

