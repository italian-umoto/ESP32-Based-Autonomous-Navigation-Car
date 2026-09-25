#include <Arduino.h>

const int IR_LED_PIN = 5;
const int SENSOR_PIN = 4;

const int COLLISION_THRESHOLD = 900;
const int NUM_SAMPLES = 4;

int readAverage() {
    long sum = 0;

    for (int i = 0; i < NUM_SAMPLES; i++) {
        sum += analogRead(SENSOR_PIN);
    }

    return sum / NUM_SAMPLES;
}

void setup() {
    Serial.begin(115200);

    pinMode(IR_LED_PIN, OUTPUT);
    digitalWrite(IR_LED_PIN, LOW);

    analogReadResolution(12);
    analogSetPinAttenuation(SENSOR_PIN, ADC_11db);

    delay(1000);
}

void loop() {
    digitalWrite(IR_LED_PIN, LOW);
    delayMicroseconds(300);
    int background1 = readAverage();
    
    digitalWrite(IR_LED_PIN, HIGH);
    delayMicroseconds(300);
    int illuminated = readAverage();
    
    digitalWrite(IR_LED_PIN, LOW);
    delayMicroseconds(300);
    int background2 = readAverage();
    
    int background = (background1 + background2) / 2;
    int reflected = illuminated - background;
    bool imminent_collision = false;
    if (reflected > COLLISION_THRESHOLD) imminent_collision = true;

    Serial.printf(
        "background: %4d  illuminated: %4d  reflected: %4d  %s\n",
        background,
        illuminated,
        reflected,
        imminent_collision ? "ABOUT TO COLLIDE" : "SAFE"
    );
    delay(50);
}
