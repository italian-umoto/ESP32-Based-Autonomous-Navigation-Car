#include "motors.h"
#include "motor_calibration.h"
#include <Arduino.h>


// https://www.pololu.com/product-info-merged/1523
// Counts per motor shaft rev: 12
// Gear ratio: 119.76
// Counts per output shaft rev: 1437.1
// Emperical: 1425


struct calib_entry {
    float speed;
    float counts_per_second; // positive
};

calib_entry motor_calib_left_forward[] = {
    {0.0f,    0.0f},
    {250.0f,  0.0f},
    {500.0f,  0.0f},
    {750.0f,  0.0f},
    {1000.0f, 0.0f},
    {1250.0f, 740.0f},
    {1500.0f, 1019.0f},
    {1750.0f, 1247.0f},
    {2000.0f, 1436.0f},
    {2250.0f, 1583.0f},
    {2500.0f, 1725.0f},
    {2750.0f, 1825.0f},
    {3000.0f, 1924.0f},
    {3250.0f, 2000.0f},
    {3500.0f, 2118.0f},
    {3750.0f, 2164.0f},
    {4000.0f, 2242.0f},
    {4250.0f, 2285.0f},
    {4500.0f, 2337.0f},
    {4750.0f, 2428.0f},
};

calib_entry motor_calib_right_forward[] = {
    {0.0f,    0.0f},
    {250.0f,  0.0f},
    {500.0f,  0.0f},
    {750.0f,  0.0f},
    {1000.0f, 0.0f},
    {1250.0f, 479.0f},
    {1500.0f, 756.0f},
    {1750.0f, 991.0f},
    {2000.0f, 1207.0f},
    {2250.0f, 1343.0f},
    {2500.0f, 1527.0f},
    {2750.0f, 1664.0f},
    {3000.0f, 1785.0f},
    {3250.0f, 1892.0f},
    {3500.0f, 2001.0f},
    {3750.0f, 2107.0f},
    {4000.0f, 2201.0f},
    {4250.0f, 2283.0f},
    {4500.0f, 2347.0f},
    {4750.0f, 2432.0f},
};

calib_entry motor_calib_left_reverse[] = {
    {0.0f,    0.0f},
    {250.0f,  0.0f},
    {500.0f,  0.0f},
    {750.0f,  173.0f},
    {1000.0f, 457.0f},
    {1250.0f, 761.0f},
    {1500.0f, 1057.0f},
    {1750.0f, 1289.0f},
    {2000.0f, 1473.0f},
    {2250.0f, 1632.0f},
    {2500.0f, 1769.0f},
    {2750.0f, 1913.0f},
    {3000.0f, 2000.0f},
    {3250.0f, 2112.0f},
    {3500.0f, 2165.0f},
    {3750.0f, 2243.0f},
    {4000.0f, 2311.0f},
    {4250.0f, 2403.0f},
    {4500.0f, 2428.0f},
    {4750.0f, 2436.0f},
};

calib_entry motor_calib_right_reverse[] = {
    {0.0f,    0.0f},
    {250.0f,  0.0f},
    {500.0f,  0.0f},
    {750.0f,  0.0f},
    {1000.0f, 0.0f},
    {1250.0f, 509.0f},
    {1500.0f, 793.0f},
    {1750.0f, 1025.0f},
    {2000.0f, 1235.0f},
    {2250.0f, 1405.0f},
    {2500.0f, 1562.0f},
    {2750.0f, 1700.0f},
    {3000.0f, 1820.0f},
    {3250.0f, 1956.0f},
    {3500.0f, 2083.0f},
    {3750.0f, 2179.0f},
    {4000.0f, 2281.0f},
    {4250.0f, 2372.0f},
    {4500.0f, 2466.0f},
    {4750.0f, 2544.0f},
};

int calib_entry_count = 20;

float calib_table_inverse_lookup(calib_entry* entry_table, float counts_per_second) {
    float prev_table_cps = entry_table[0].counts_per_second;
    if (counts_per_second <= prev_table_cps) {
        return entry_table[0].speed;
    }
    for (int i = 1; i < calib_entry_count; i++) {
        float curr_table_cps = entry_table[i].counts_per_second;
        if (curr_table_cps >= counts_per_second) {
            float fraction = (counts_per_second - prev_table_cps) / (curr_table_cps - prev_table_cps);
            return entry_table[i-1].speed + fraction * (entry_table[i].speed - entry_table[i-1].speed );
        }
        prev_table_cps = curr_table_cps;
    }
    return entry_table[calib_entry_count-1].speed;
}

float feedforward_left_speed(float cps) {
    if (cps > 0) {
        return calib_table_inverse_lookup(motor_calib_left_forward, cps);
    } else {
        return -calib_table_inverse_lookup(motor_calib_left_reverse, -cps);
    }
}


float feedforward_right_speed(float cps) {
    if (cps > 0) {
        return calib_table_inverse_lookup(motor_calib_right_forward, cps);
    } else {
        return -calib_table_inverse_lookup(motor_calib_right_reverse, -cps);
    }
}

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
