#include "lane_following.h"
#include "drive.h"
#include "color_sensor.h"
#include <Arduino.h>

/* This algorithm assumes that there are two color sensors
 * on the robot and that they are about the width of the lane apart
 */
static LaneStatus lastStatus = LaneStatus::Lost;
static uint32_t lastStepMs = 0;
static uint32_t settleUntilMs = 0;

constexpr float KP = 1.0f; // TUNABLE
constexpr float KI = 0.0f; // TUNABLE
constexpr float KD = 0.0f; // TUNABLE
constexpr float I_LIMIT = 50.0f; // TUNABLE

// These probably shouldn't be used -- see comment over calcPid()
// static float prevErr1 = 0.0f;
// static float prevErr2 = 0.0f;
// static float lastOutput = 0.0f;
static float prevErr = 0.0f;
static float integral = 0.0f;

constexpr float MIN_RADIUS_MM = 80.0f; // TUNABLE
constexpr float STRAIGHT_RADIUS_MM = 1e6f; 
constexpr float APPROX_STRAIGHT_THRESH = 1e-4f; 
constexpr uint32_t LANE_PERIOD_MS = 10; // TUNABLE
constexpr uint32_t SETTLE_MS = 50; // TUNABLE
constexpr float LOST_THRESHOLD = 0.15f; // TUNABLE

static bool active = false;why does 
static DetectedColor currentTarget = DetectedColor::Dark;



static void steer(float output) {
    const float mag = fabsf(output);
    // If we are on track, we take a basically straight path which simplifies
    // the code and keeps the movement *hopefully* smoother assuming mag != 0.
    //
    // MIN_RADIUS_MM is set to be the tightest turn we could take to stay on 
    // a line, mag is always a fraction or +/-1, so larger mag => closer to
    // tightest turn
    const float radius = (mag < APPROX_STRAIGHT_THRESH) ? STRAIGHT_RADIUS_MM
                                                        : MIN_RADIUS_MM / mag;
                        
    if (output > 0) drive_turn_left(radius);
    else drive_turn_right(radius);
}

// I am unsure of which version is better here:
//    return (error - prevErr1) * KP
            // + (error - (2 * prevErr1) + prevErr2) * KD 
            // + error * KI 
            // + lastOutput;
// the above is a choice copied from motor module, 
// but the lane detector is a little different in that
// lastOutput is clamped when the line is lost, making
// the proportional value lose meaning
static float calcPid(float error) {
    // KI * I_LIMIT gives the maximum correction that can be applied by the integral term.
    // We constrain to keep it within reasonable bounds
    integral = constrain(integral + error, -I_LIMIT, I_LIMIT);

    // The proportion steers relative to how far off of the line
    // the robot is right now, this has large impact
    float output = KP * error
    // The integral steers based on error over time (small change to correct drift)
                   + KI * integral
    // The derivative steers based on how fast the error changes (if error is shrinking and robot is moving toward line, the derivative damps the steering)
                   + KD * (error - prevErr);
    prevErr = error;
    // Everything is clamped to a percentage and a sign
    return constrain(output, -1.0f, 1.0f);
}

LaneStatus followLane(DetectedColor color) {
    const uint32_t now = millis();
    if (!active || color != currentTarget) { 
        leftColorSensor.startTracking(color); 
        rightColorSensor.startTracking(color);
        currentTarget = color;
        active = true;

        prevErr = 0.0f;
        integral = 0.0f;
        settleUntilMs = now + SETTLE_MS;
        lastStepMs = now;
        lastStatus = LaneStatus::Following;
    }

    // These two cases are here to allow time for the sensors
    // to hit valid output and to normalize when the pid calculation 
    // happens. The assumption is made that this code will run frequently 
    // enough that we will almost always hit it around every LANE_PERIOD_MS.
    // This allows us to not include dt in our PID calculation because we
    // assume a constant interval. If this is not the case, we can either
    // change the calculation or put this on a timer.
    if (now < settleUntilMs) return lastStatus;
    if (now - lastStepMs < LANE_PERIOD_MS) return lastStatus;
    lastStepMs = now;

    // We get values between 0 and 1 from each sensor 0 => black, 1 => color 
    float l = leftColorSensor.lineStrength();
    float r = rightColorSensor.lineStrength();

    float output;
    if (l + r < LOST_THRESHOLD) {
        // This case occurs when both color sensors hit mostly black, 
        // meaning we are not on a line anymore and we should just 
        // turn towards wherever we last saw some of the target color.
        output = (prevErr >= 0.0f) ? 1.0f : -1.0f;
        lastStatus = LaneStatus::Lost;
    } else {
        // The value passed into calcPid is chosen to create a normalization
        // of the reading that is agnostic to any conditions that equally
        // scale the outputs of the sensors.
        // 
        // Let's say the LEDs all dim by the same percentage, readings of 
        // 0.75 and 0.25 => 0.6 and 0.2. (l - r)/(l + r) is 0.5 in both cases
        // making the change to the steering the same in both cases which is 
        // good because it allows for variation in testing conditions to not 
        // require drastically different pid calculations.
        output = calcPid((l - r) / (l + r));
        lastStatus = LaneStatus::Following;
    }

    steer(output);
    return lastStatus;
}

void stopLaneFollowing() {
    leftColorSensor.stopTracking();
    rightColorSensor.stopTracking();
    active = false;
}

