"""Check encoder-limited pivots using simulated motor/encoder I/O."""
from pathlib import Path
import subprocess
import tempfile
import unittest


class DriveTurnTest(unittest.TestCase):
    def test_pivots(self):
        root = Path(__file__).resolve().parents[1]
        with tempfile.TemporaryDirectory(prefix="ee31-drive-") as directory:
            temp = Path(directory)
            (temp / "esp_err.h").write_text("#pragma once\ntypedef int esp_err_t;\n")
            (temp / "test.cpp").write_text(r'''
#include "drive.h"
#include <cassert>
#include <cstdio>
int encL=0, encR=0;
float motorL=0, motorR=0;
int left_encoder_count(){return encL;}
int right_encoder_count(){return encR;}
void motors_command(float l,float r){motorL=l;motorR=r;}
void motors_stop(){motorL=motorR=0;}
int main(){
    // CW pivot: mirrored left motor and right motor both command negative.
    assert(!drive_turn(90,0,0)); assert(motorL<0 && motorR<0);
    encL=-1000; assert(!drive_turn(90,0,0)); assert(motorL==0 && motorR<0);
    encR=-1000; assert(drive_turn(90,0,0)); assert(motorL==0 && motorR==0);
    encL=encR=0;
    assert(!drive_turn(-90,0,0)); assert(motorL>0 && motorR>0);
    encL=encR=1000; assert(drive_turn(-90,0,0));

    // Entry counts must be captured afresh after any forward travel.
    encL=-647; encR=647;
    assert(drive_turn(0,encL,encR));
    const int startL=encL, startR=encR;
    assert(!drive_turn(90,startL,startR));
    encL-=1000; encR-=1000;
    assert(drive_turn(90,startL,startR));
    puts("Passed pivots, independent wheel stopping, entry offsets, and zero angle.");
}
''')
            subprocess.run([
                "c++", "-std=c++17", "-I" + str(temp), "-I" + str(root / "include"),
                str(root / "src/drive.cpp"), str(temp / "test.cpp"),
                "-o", str(temp / "test"),
            ], check=True)
            subprocess.run([str(temp / "test")], check=True)


if __name__ == "__main__":
    unittest.main()
