#pragma once
#include "color_sensor.h"

/* Motion
 *
 * Everything the robot can do
 * 0-6 match the websocket commands 7 and 8 are extensions
 * for the route following
 */
enum class Motion : int {
  Stop = 0,
  Forward = 1,    // param: speed (cps)
  Backward = 2,   // param: speed (cps)
  PivotCW = 3,    // param: speed (cps)
  PivotCCW = 4,   // param: speed (cps)
  TurnRight = 5,  // param: radius (mm)
  TurnLeft = 6,   // param: radius (mm)
  Rotate = 7,     // param: degrees (+ CW, - CCW)
  FollowLane = 8, // color: lane to follow
};

enum class BehaviorStatus {
  Running,
  Done,
};

class Behavior {
public:
  /* start
   *
   * Entry action: exit the current motion then begin the new one
   */
  void start(Motion motion, float param = 0.0f,
             DetectedColor color = DetectedColor::Dark);

  /* tick
   *
   * Run one step of the current motion
   *
   * Return: Done when a self-finishing motion (Rotate) completes,
   *         or for Stop. Running otherwise
   */
  BehaviorStatus tick();

  Motion current() const { return m_motion; }

  // True for motions that move the front of the robot toward a wall
  // Used by the FSM's remote-mode safety stop on wall
  bool movesForward() const;

private:
  void exitCurrent();

  Motion m_motion = Motion::Stop;
};
