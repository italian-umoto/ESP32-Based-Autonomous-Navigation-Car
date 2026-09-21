#include "motors.h"
#include "motor_calibration.h"
#include <Arduino.h>


// https://www.pololu.com/product-info-merged/1523
// Counts per motor shaft rev: 12
// Gear ratio: 119.76
// Counts per output shaft rev: 1437.1
// Emperical: 1425



void run_motor_test() {


    Serial.println("Begin motor speed test (speed -> counts / second)");

    for (int mode = 0; mode < 2; mode ++) {
        if (mode == 0) {
            bdc_motor_forward(&left_motor);
            bdc_motor_forward(&right_motor);
            Serial.println("Forward:");
        } else {
            bdc_motor_reverse(&left_motor);
            bdc_motor_reverse(&right_motor);
            Serial.println("Reverse:");
        }


        for (int speed = 0; speed < BDC_MCPWM_DUTY_TICK_MAX; speed += BDC_MCPWM_DUTY_TICK_MAX / 20) {
            bdc_motor_set_speed(&left_motor, speed);
            bdc_motor_set_speed(&right_motor, speed);

            delay(500); // wait for steady state
            
            int start_left = left_encoder_count();
            int start_right = right_encoder_count();

            delay(1000);
            int end_left = left_encoder_count();
            int end_right = right_encoder_count();

            Serial.print("Speed = ");
            Serial.print(speed);
            Serial.print(": left = ");
            Serial.print(end_left - start_left);
            Serial.print(", right = ");
            Serial.println(end_right - start_right);
        }

    }


}