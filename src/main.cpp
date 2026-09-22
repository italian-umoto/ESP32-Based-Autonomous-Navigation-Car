#include <Arduino.h>

// ESP32-S3 wiring: photoresistor voltage divider to GPIO4 (ADC1),
// and red, green, blue LEDs (each with a current-limiting resistor)
// to GPIO5, GPIO6, and GPIO7. Keep the sensor and target shaded
// from changing room light for more repeatable readings.
constexpr int SENSOR_PIN = 4;
constexpr int RED_LED_PIN = 5;
constexpr int GREEN_LED_PIN = 6;
constexpr int BLUE_LED_PIN = 7;
constexpr int NUM_SAMPLES = 16;
constexpr int DARK_THRESHOLD = 80;     // ADC counts above ambient; tune for your setup
constexpr float COLOR_DOMINANCE = 1.25f; // winning channel must exceed the runner-up
constexpr int DELAY_MS = 10;

int readAverage() {
    long total = 0;
    for (int i = 0; i < NUM_SAMPLES; ++i) {
        total += analogRead(SENSOR_PIN);
        delayMicroseconds(100);
    }
    return total / NUM_SAMPLES;
}

// Sets one of R/G/B high and gets the reading minus the ambient light
int readReflection(int ledPin, int ambient) {
    digitalWrite(ledPin, HIGH);
    delay(DELAY_MS);
    const int illuminated = readAverage();
    digitalWrite(ledPin, LOW);
    // Subtract ambient light and avoid negative readings.
    return max(0, illuminated - ambient);
}

const char *detectColor(int red, int green, int blue) {
    const int brightest = max(red, max(green, blue));
    if (brightest < DARK_THRESHOLD) return "black/dark";

    if (red > green * COLOR_DOMINANCE && red > blue * COLOR_DOMINANCE) return "red";
    if (green > red * COLOR_DOMINANCE && green > blue * COLOR_DOMINANCE) return "green";
    if (blue > red * COLOR_DOMINANCE && blue > green * COLOR_DOMINANCE) return "blue";
    return "mixed/white";
}

void setup() {
    Serial.begin(115200);
    pinMode(RED_LED_PIN, OUTPUT);
    pinMode(GREEN_LED_PIN, OUTPUT);
    pinMode(BLUE_LED_PIN, OUTPUT);

    // return values between 0 and 4095
    analogReadResolution(12);

    // configure ADC to measure higher voltages
    analogSetPinAttenuation(SENSOR_PIN, ADC_11db);
    delay(1000);
}

void loop() {
    digitalWrite(RED_LED_PIN, LOW);
    digitalWrite(GREEN_LED_PIN, LOW);
    digitalWrite(BLUE_LED_PIN, LOW);

    delay(DELAY_MS);
    const int ambient = readAverage();
    delay(DELAY_MS);
    const int green = readReflection(GREEN_LED_PIN, ambient);
    delay(DELAY_MS);
    const int red = readReflection(RED_LED_PIN, ambient);
    delay(DELAY_MS);
    const int blue = readReflection(BLUE_LED_PIN, ambient);

    Serial.print("R: "); Serial.print(red);
    Serial.print("  G: "); Serial.print(green);
    Serial.print("  B: "); Serial.print(blue);
    Serial.print("  color: "); Serial.println(detectColor(red, green, blue));
    delay(100);
}
