#pragma once

struct colorReading{
    int red;
    int green;
    int blue;
};

enum class DetectedColor{
    Dark,
    Red,
    Yellow,
    Blue,
    White
};

// Values for black and line color gathered via
// shining LED of specified color over said color
// and over black. Stored in .cpp file in 
// LEFT_CAL and RIGHT_CAL for respective sensor.
// Are stored and calibrated at test time
struct ChannelCal {
    int black;
    int line;
};

// These are the set points updated in testing 
// basically hardcoded values
struct SensorCal {
    ChannelCal red;
    ChannelCal yellow;
    ChannelCal blue;
};

class ColorSensor {
    public:
        ColorSensor(int sensorPin, int redPin, int greenPin, int bluePin,
                    const SensorCal &cal);
        void colorSensorInit();

        // Members for discrete reads (flash LEDs and classify)
        colorReading singleRead();
        DetectedColor classifyColor(const colorReading& reading);
        void testReading();

        // Members for continuous reads (LED stays on for longer/get analog val)
        void startTracking(DetectedColor target);
        void stopTracking();

        // Returns 0 (black) -> 1 (color) depending on how much of the
        // sensor is over a color
        float lineStrength();

        void testTracking(DetectedColor target);

    private:
        void printReading(const colorReading& reading, DetectedColor color);
        void resetLED();
        int readAverage();
        int readReflection(int ledPin, int ambient);

        // Gives pin number for specified color
        int ledPinFor(DetectedColor target) const;

        // Gives calibrated values (black, color) for specified color
        const ChannelCal *calFor(DetectedColor target) const;

        const int m_sensor_pin;
        const int m_red_pin;
        const int m_green_pin;
        const int m_blue_pin;
        const SensorCal m_cal;

        bool m_tracking = false;
        DetectedColor m_target = DetectedColor::Dark;
};


// Global Color Sensors for use in lane following module
extern ColorSensor leftColorSensor;
extern ColorSensor rightColorSensor;
void setupColorSensors();

