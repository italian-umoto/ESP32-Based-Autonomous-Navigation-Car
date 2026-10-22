/***
 * Battery management
 * 
 * Handles battery level detection using an analog pin and a moving average
 * filter
 */

#pragma once

#define BAT_VOLTAGE_PIN 3
#define BAT_STATUS_LED_PIN 11
#define THRESHOLD_VOLTAGE 9.0
#define R_SHUNT 2.56E6
#define R_DIV 4.7E6


typedef enum BatteryLevel {
    GOOD=0, REPLACE=1
} BatteryLevel;

void batteryInit();
BatteryLevel checkBatteryLevel();