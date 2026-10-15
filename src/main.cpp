#include "color_sensor.h"
#include "websocket.h"
#include "collision.h"
#include "motors.h"
#include "drive.h"
#include "fsm.h"

FSM fsm;

void setup() {
    Serial.begin(115200);
    setup_motors();
    // websocketInit();
    colorInit();
    setupCollisionDetection();
}


DetectedColor target_color = DetectedColor::Red;
int detected_in_phase_count = 0;
bool on_phase = false;
long swap_phase_millis = 0;
long swap_period = 300;

void loop() {
    // fsm.tick();

    // targets left side of lane

    
    const colorReading reading = single_read();
    const DetectedColor color = classifyColor(reading);
    printReading(reading, color);

    long now = millis();
    bool detected_in_phase = (detected_in_phase_count > 1) && on_phase || (detected_in_phase_count > 3) && (!on_phase);

    if (swap_phase_millis < now && detected_in_phase) {
        on_phase = !on_phase;
        swap_phase_millis = now + swap_period;
        detected_in_phase_count = 0;
        Serial.println("swap");

    }

    if (!detected_in_phase) {
        bool on_color = (target_color == color);
        if (on_phase) {
            // turn right until on color
            drive_wheels(300, 100);


            if (on_color) {
                Serial.print("on count ");
                Serial.println(detected_in_phase_count);

                detected_in_phase_count ++;
            }
        } else {
            // turn left until off color
            drive_wheels(100, 300);


            if (!on_color) {
                Serial.print("off count ");
                Serial.println(detected_in_phase_count);

                detected_in_phase_count ++;
            }
        }
    } else {
        drive_wheels(300, 300);

    }


}

