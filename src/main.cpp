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
    websocketInit();

    //setup_motors();
    // bdc_motor_forward(&left_motor);
    // bdc_motor_forward(&right_motor);
    // bdc_motor_set_speed(&left_motor, BDC_MCPWM_DUTY_TICK_MAX / 2);
    // bdc_motor_set_speed(&right_motor, BDC_MCPWM_DUTY_TICK_MAX / 2);
    // delay (1000);
    // run_motor_test();

    //enable_pid();

}

void loop() {

    //left_set_speed_pid(2000);
    //right_set_speed_pid(-2000);
    //for (int i =0;i<20; i ++){
    //    print_pid_debug();
    //    delay(100);
    //}
    //left_set_speed_pid(000);
    //right_set_speed_pid(000);
    //for (int i =0;i<20; i ++){
    //    print_pid_debug();
    //    delay(100);
    //}
    //left_set_speed_pid(-2000);
    //right_set_speed_pid(2000);
    //for (int i =0;i<20; i ++){
    //    print_pid_debug();
    //    delay(100);
    //}

    fsm.tick();

    delay(100); 
}

