#include "fsm.h"
#include <WiFi.h>
#include <WebSocketsClient.h>
#include "websocket.h"
#include "motor_calibration.h"
#include "motors.h"

Command command;
FSM fsm;

void setup() {
    Serial.begin(115200);
    // websocketInit();

    setup_motors();
    // bdc_motor_forward(&left_motor);
    // bdc_motor_forward(&right_motor);
    // bdc_motor_set_speed(&left_motor, BDC_MCPWM_DUTY_TICK_MAX / 2);
    // bdc_motor_set_speed(&right_motor, BDC_MCPWM_DUTY_TICK_MAX / 2);
    delay (1000);
    run_motor_test();

}

void loop() {
    // Serial.print("Left: ");
    // Serial.print(left_encoder_count());
    // Serial.println("");
    // Serial.print("Right: ");
    // Serial.print(right_encoder_count());
    // Serial.println("");

    fsm.tick();
    // bool ret = getCommand(command);
    // if (ret) {
    //     Serial.println("New Command:");
    //     Serial.print("type: ");
    //     Serial.println(commandTypeToString(command.type));
    //     Serial.print("value: ");
    //     Serial.println(command.value);
    // }
    delay(1000); 
}

