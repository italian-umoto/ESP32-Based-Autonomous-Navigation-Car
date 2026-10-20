#include <Arduino.h>
#include "battery.h"
#include <queue>

#define SAMPLES 10
#define VOLT_STEP 3.3 / 4096
#define VOLT_MULT_RATIO (R_SHUNT + R_DIV) / R_SHUNT

double filtered_voltage;

void batteryInit() {
    pinMode(BAT_VOLTAGE_PIN, INPUT);
    pinMode(BAT_STATUS_LED_PIN, OUTPUT);
}

BatteryLevel checkBatteryLevel() {
    double voltage = analogRead(BAT_VOLTAGE_PIN) * VOLT_STEP * VOLT_MULT_RATIO;

    filtered_voltage = (filtered_voltage * (SAMPLES - 1) + voltage) / SAMPLES;

    bool bat_stat = (filtered_voltage < THRESHOLD_VOLTAGE);
    char str[50] = "";
    snprintf(str, 50, "Battery status: %.4fV\n", filtered_voltage);
    Serial.write(str);
    digitalWrite(BAT_STATUS_LED_PIN, bat_stat? HIGH : LOW);
    return (BatteryLevel) bat_stat;
}
