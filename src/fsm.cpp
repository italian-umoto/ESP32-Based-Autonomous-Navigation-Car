#include "fsm.h"
#include <stdint.h>
#include <Arduino.h> 
#include "websocket.h"
#include "motors.h"


static const char *TAG = "MOTOR";

#define STATE_INDICATOR_LED RGB_BUILTIN
#define BRIGHT 64
#define STATE_TRANSITION_PIN 5
#define DEBOUNCE_DELAY 10


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
    if (state < 0 || state >= numStates) {
        Serial.print("Invalid state: ");
        Serial.println(state);
        return;
    }
    this->state = state;
    currentState = statesList[state];
}

void FSM::checkCommandUpdate() {
    Command cmd;
    if (getCommand(cmd)) {
        updateState(cmd.value, cmd.altValue);
    } else if (buttonPressed()) {
        updateState(this->state + 1, 0);
    }
} 




//               PIN          RED     GREEN  BLUE
// rgbLedWrite(RGB_BUILTIN, BRIGHT, 0,     0);
void FSM::stateStop() {
    checkCommandUpdate();
    Serial.println("In Idle state :)");
    rgbLedWrite(RGB_BUILTIN, BRIGHT, 0, 0);
    ESP_ERROR_CHECK_WITHOUT_ABORT(bdc_motor_brake(&left_motor));
    ESP_ERROR_CHECK_WITHOUT_ABORT(bdc_motor_brake(&right_motor));
    delay(1000);
}

void FSM::stateForward() {
    checkCommandUpdate();
    Serial.println("In state one");
    rgbLedWrite(RGB_BUILTIN, 0, BRIGHT, 0);
    ESP_ERROR_CHECK_WITHOUT_ABORT(bdc_motor_forward(&left_motor));
    ESP_ERROR_CHECK_WITHOUT_ABORT(bdc_motor_forward(&right_motor));
    delay(1000);
}

void FSM::stateBackward() {
    checkCommandUpdate();
    Serial.println("In state two");
    rgbLedWrite(RGB_BUILTIN, 0, 0, BRIGHT); 
    ESP_ERROR_CHECK_WITHOUT_ABORT(bdc_motor_reverse(&left_motor));
    ESP_ERROR_CHECK_WITHOUT_ABORT(bdc_motor_reverse(&right_motor));
    delay(1000);
}

void FSM::statePivotCW() {
    checkCommandUpdate();
    Serial.println("In state three");
    rgbLedWrite(RGB_BUILTIN, BRIGHT, BRIGHT, 0);
    delay(1000);
}

void FSM::statePivotCCW() {
    checkCommandUpdate();
    Serial.println("In state four");
    rgbLedWrite(RGB_BUILTIN, BRIGHT, 0, BRIGHT);
    delay(1000);
}

void FSM::stateRightTurn() {
    checkCommandUpdate();
    Serial.println("In state five");
    rgbLedWrite(RGB_BUILTIN, 0, BRIGHT, BRIGHT);
    delay(1000);
}

void FSM::stateLeftTurn() {
    checkCommandUpdate();
    Serial.println("In state six");
    rgbLedWrite(RGB_BUILTIN, BRIGHT, BRIGHT, BRIGHT);
    delay(1000);
}

