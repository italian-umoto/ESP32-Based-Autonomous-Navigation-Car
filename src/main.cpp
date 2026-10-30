// #include "collision.h"
// #include "color_sensor.h"
// #include "fsm.h"
// #include "motors.h"
// #include "websocket.h"
//
// FSM fsm;
//
// void setup() {
//   Serial.begin(115200);
//   setupColorSensors();
//   // setup_motors();
//   //  websocketInit();
//   // setupCollisionDetection();
// }
//
// void loop() {
//   Serial.print("LEFT   ");
//   leftColorSensor.testReading();
//   Serial.print("RIGHT  ");
//   rightColorSensor.testReading();
//   Serial.println();
//   delay(200);
//   // fsm.tick();
//   // delay(10);
// }
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
// Lane following test for a single color.
// Set the robot on the line during the startup delay. Over serial:
//   s  stop (sensors off, motors stopped)
//   g  start again

#include "color_sensor.h"
#include "lane_following.h"
#include "motors.h"
#include <Arduino.h>

constexpr DetectedColor TARGET = DetectedColor::Red;
constexpr unsigned long START_DELAY_MS = 3000;
constexpr unsigned long PRINT_PERIOD_MS = 200;

bool running = false;

void handleSerial() {
  if (!Serial.available())
    return;

  switch (Serial.read()) {
  case 's':
    laneStop();
    running = false;
    Serial.println("STOPPED");
    break;
  case 'g':
    laneStart(TARGET);
    running = true;
    Serial.println("STARTED");
    break;
  }
}

void setup() {
  Serial.begin(115200);
  setupColorSensors();
  setup_motors();

  Serial.println("Lane test: s = stop, g = go");
  delay(START_DELAY_MS); // time to set the robot on the line

  laneStart(TARGET);
  running = true;
}

void loop() {
  handleSerial();
  if (!running)
    return;

  // Call every loop with no delay; laneStep handles its own 10 ms period.
  LaneStatus status = laneStep();

  static uint32_t lastPrint = 0;
  if (millis() - lastPrint >= PRINT_PERIOD_MS) {
    lastPrint = millis();
    Serial.printf("L %.2f  R %.2f  %s\n", leftColorSensor.lineStrength(),
                  rightColorSensor.lineStrength(),
                  status == LaneStatus::Lost ? "LOST" : "following");
  }
}
//
//
//
//
//
//

// #include "color_sensor.h"
// #include <Arduino.h>
//
// constexpr unsigned long PRINT_PERIOD_MS = 200;
//
// ColorSensor *sensor = &leftColorSensor;
// const char *sensorName = "LEFT";
// DetectedColor target = DetectedColor::Red;
// const char *targetName = "red";
//
// // LEDs are shared between the sensors, so turn everything off on any change
// // and let the next testTracking() call start fresh.
// void resetBoth() {
//   leftColorSensor.stopTracking();
//   rightColorSensor.stopTracking();
// }
//
// void handleSerial() {
//   if (!Serial.available())
//     return;
//
//   switch (Serial.read()) {
//   case 'l':
//     sensor = &leftColorSensor;
//     sensorName = "LEFT";
//     break;
//   case 'R':
//     sensor = &rightColorSensor;
//     sensorName = "RIGHT";
//     break;
//   case 'r':
//     target = DetectedColor::Red;
//     targetName = "red";
//     break;
//   case 'y':
//     target = DetectedColor::Yellow;
//     targetName = "yellow";
//     break;
//   case 'b':
//     target = DetectedColor::Blue;
//     targetName = "blue";
//     break;
//   default:
//     return; // ignore newlines and anything else
//   }
//
//   resetBoth();
//   Serial.printf("\n--- %s sensor, %s ---\n", sensorName, targetName);
// }
//
// void setup() {
//   Serial.begin(115200);
//   delay(1000); // give the USB serial monitor time to attach
//
//   setupColorSensors();
//
//   Serial.println(
//       "Calibration: l/R = LEFT/RIGHT sensor, r/y/b = red/yellow/blue");
//   Serial.printf("--- %s sensor, %s ---\n", sensorName, targetName);
// }
//
// void loop() {
//   handleSerial();
//
//   Serial.printf("%-5s %-6s ", sensorName, targetName);
//   sensor->testTracking(target);
//
//   delay(PRINT_PERIOD_MS);
// }
