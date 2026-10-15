#include <Arduino.h>
#include "battery.h"
#include <queue>

#define VOLT_STEP 3.3 / 4096
#define QUEUE_LEN 10

// std::queue<double> q;
int samples = 0;
double filtered_voltage;

void batteryInit() {
    pinMode(BAT_VOLTAGE_PIN, INPUT);
    pinMode(BAT_STATUS_LED_PIN, OUTPUT);
}

BatteryLevel checkBatteryLevel() {
    double voltage = analogRead(BAT_VOLTAGE_PIN) * VOLT_STEP;
    // double voltage_to_remove = 0.0;

    // q.push(voltage);
    // if (samples <= QUEUE_LEN) {
    //     samples++;
    // } else {
    //     double voltage_to_remove = q.front();
    //     q.pop();
    // }
    filtered_voltage = (filtered_voltage * (QUEUE_LEN - 1) + voltage) / QUEUE_LEN;

    bool bat_stat = (filtered_voltage < THRESHOLD_VOLTAGE);
    char str[50] = "";
    snprintf(str, 50, "Battery status: %.4fV\n", filtered_voltage);
    Serial.write(str);
    digitalWrite(BAT_STATUS_LED_PIN, bat_stat? HIGH : LOW);
    return (BatteryLevel) bat_stat;
}
