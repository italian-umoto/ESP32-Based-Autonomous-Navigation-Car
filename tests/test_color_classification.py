"""Replay measured normalized readings against the real C++ classifier.
Run: python3 tests/test_color_classification.py (requires a host C++ compiler).
"""
from pathlib import Path
import subprocess
import tempfile
import unittest

# R, G, B, intensity: measured samples from both sensors, including repeats.
SAMPLES = {
    "Red": """
.780 .105 .115 1.475
.856 .093 .050 1.475
.776 .100 .124 1.546
.854 .095 .051 1.449
.772 .102 .126 1.521
.866 .082 .052 1.429
.783 .103 .113 1.500
.866 .082 .052 1.429
.785 .099 .116 1.465
.856 .086 .058 1.476
.769 .103 .128 1.496
.851 .090 .060 1.423
.761 .105 .135 1.578
.850 .086 .064 1.486
""",
    "Yellow": """
.632 .337 .031 4.828
.694 .292 .014 4.627
.632 .337 .031 4.828
.694 .295 .011 4.626
.635 .342 .022 4.760
.698 .293 .009 4.488
.634 .338 .027 4.690
.696 .294 .009 4.498
.632 .340 .027 4.665
.698 .290 .012 4.525
.636 .341 .023 4.679
.698 .293 .009 4.488
.632 .340 .027 4.665
.696 .294 .009 4.498
.632 .340 .027 4.665
.699 .293 .007 4.477
.634 .338 .027 4.690
""",
    "Blue": """
.106 .317 .577 3.316
.184 .485 .332 1.861
.098 .322 .580 3.302
.172 .480 .348 1.836
.099 .319 .582 3.291
.183 .471 .346 1.873
.106 .317 .577 3.316
.171 .483 .346 1.846
.092 .320 .588 3.256
.188 .485 .327 1.820
.116 .327 .557 3.245
.171 .482 .347 1.688
""",
    "Dark": """
.528 .191 .281 .379
.836 .062 .101 .315
.685 .103 .212 .401
.781 .132 .086 .370
.648 .187 .165 .386
.803 .109 .089 .361
.576 .178 .245 .347
.765 .142 .093 .344
.589 .189 .223 .382
.744 .166 .090 .354
""",
}

ARDUINO_STUB = r'''
#pragma once
#include <algorithm>
#include <stdint.h>
using std::min; using std::max;
#define LOW 0
#define HIGH 1
#define INPUT 0
#define OUTPUT 1
#define ADC_11db 3
template<class T> T constrain(T v,T a,T b){return min(max(v,a),b);}
inline void pinMode(int,int){} inline void digitalWrite(int,int){}
inline int analogRead(int){return 0;} inline void delayMicroseconds(int){}
inline void analogReadResolution(int){} inline void analogSetPinAttenuation(int,int){}
struct SerialStub { template<class... T> void printf(const char*,T...){} };
inline SerialStub Serial;
'''


class ColorClassificationTest(unittest.TestCase):
    def test_measured_samples_and_variation(self):
        rows = []
        for name, samples in SAMPLES.items():
            for line in samples.strip().splitlines():
                r, g, b, intensity = line.split()
                rows.append("{{%sf,%sf,%sf,0,0,0,%sf},DetectedColor::%s}" %
                            (r, g, b, intensity, name))
        source = r'''
#include "color_sensor.h"
#include <cassert>
#include <cstdio>
struct Sample {colorReading reading; DetectedColor expected;};
Sample samples[] = {FIXTURES};
void check(const colorReading& reading, DetectedColor expected) {
    assert(classifyColor(reading) == expected);
    for (auto target : {DetectedColor::Red, DetectedColor::Blue, DetectedColor::Yellow}) {
        const float strength = targetStrength(reading, target);
        assert(strength >= 0 && strength <= 1);
        if (target == expected) assert(strength > 0.05f);
        else assert(strength == 0);
    }
}
int main() {
    for (const auto& sample : samples) {
        check(sample.reading, sample.expected);
        // Change one channel by 0.02, renormalize, and vary brightness by 20%.
        for (int channel = 0; channel < 3; ++channel)
            for (float delta : {-0.02f, 0.02f})
                for (float scale : {0.8f, 1.2f}) {
                    auto varied = sample.reading;
                    float values[] = {varied.red, varied.green, varied.blue};
                    values[channel] = std::max(0.0f, values[channel] + delta);
                    const float total = values[0] + values[1] + values[2];
                    varied.red = values[0] / total;
                    varied.green = values[1] / total;
                    varied.blue = values[2] / total;
                    varied.intensity *= scale;
                    check(varied, sample.expected);
                }
    }
    check({0,0,0,0,0,0,0}, DetectedColor::Dark);
    check({.4f,.4f,.2f,0,0,0,2}, DetectedColor::Dark); // Ambiguous.
    printf("Passed %zu measured samples plus 636 perturbed samples.\n",
           sizeof(samples) / sizeof(samples[0]));
}
'''.replace("FIXTURES", ",\n".join(rows))
        root = Path(__file__).resolve().parents[1]
        with tempfile.TemporaryDirectory(prefix="ee31-colors-") as directory:
            temp = Path(directory)
            (temp / "Arduino.h").write_text(ARDUINO_STUB)
            (temp / "test.cpp").write_text('#include "Arduino.h"\n' + source)
            subprocess.run([
                "c++", "-std=c++17", "-I" + str(temp), "-I" + str(root / "include"),
                str(root / "src/color_sensor.cpp"), str(temp / "test.cpp"),
                "-o", str(temp / "test"),
            ], check=True)
            subprocess.run([str(temp / "test")], check=True)


if __name__ == "__main__":
    unittest.main()
