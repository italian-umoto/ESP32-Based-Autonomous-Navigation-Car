#include <Arduino.h>

constexpr int SENSOR_PIN = 4;
constexpr int RED_LED_PIN = 5;
constexpr int GREEN_LED_PIN = 6;
constexpr int BLUE_LED_PIN = 7;
constexpr int NUM_SAMPLES = 16;
constexpr int DARK_THRESHOLD = 30;
constexpr float COLOR_DOMINANCE = 1.25f; // color winner margin
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
    return max(0, illuminated - ambient);
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

    const int ambient = readAverage();
    delay(100);
    const int red = readReflection(RED_LED_PIN, ambient);
    delay(100);
    const int green = readReflection(GREEN_LED_PIN, ambient);
    delay(100);
    const int blue = readReflection(BLUE_LED_PIN, ambient);

    const int brightest = max(red, max(green, blue));
    const char* color = "white";
    if (brightest < DARK_THRESHOLD) color = "dark";
    else if (red > green * COLOR_DOMINANCE && red > blue * COLOR_DOMINANCE) color = "red";
    else if (green > blue * COLOR_DOMINANCE && red > blue * COLOR_DOMINANCE) color = "yellow";
    else if (blue > red * COLOR_DOMINANCE && blue > green * COLOR_DOMINANCE) color = "blue";
    Serial.print("R: "); Serial.print(red);
    Serial.print("  G: "); Serial.print(green);
    Serial.print("  B: "); Serial.print(blue);
    Serial.print("  color: "); Serial.println(color);
    delay(100);
}
