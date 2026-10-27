#pragma once
#include "behavior.h"
#include <stdint.h>

/* Exit
 *
 * What ends a route step
 */
enum class Exit {
  Never,
  Done,  // the Behavior reports Done
  Wall,  // collision sensor sees a wall
  Color, // both color sensors see step.color
};

/* Step
 *
 * One row of the ROUTE table
 * color is either the lane color to follow or find
 */
struct Step {
  const char *name;
  Motion motion;
  float param;
  DetectedColor color;
  Exit exit;
};

/* Route
 *
 * Upper level of the FSM: walks the step table and decides when to move
 * on. The table itself is in route.cpp
 */
class Route {
public:
  /* start
   *
   * Param:  fromStep - step index to begin at
   * Return: the step to run first
   */
  const Step &start(uint8_t fromStep = 0);

  /* tick
   *
   * Check the current step's exit condition
   *
   * Param:  wall   - this tick's collision reading
   *         status - this tick's Behavior status
   * Return: the next step if the current one just finished,
   *         nullptr otherwise
   *
   * Effects: May do a blocking color read (bothSensorsSee) on Color steps
   */
  const Step *tick(bool wall, BehaviorStatus status);

  // returns the current step
  const Step &current() const;
  // returns the current index into the ROUTE table
  uint8_t index() const { return m_idx; }

private:
  // has the exit condition been met
  bool exitMet(const Step &step, bool wall, BehaviorStatus status);

  // index into ROUTE
  uint8_t m_idx = 0;

  // counter for correct color reads
  uint8_t m_confirm = 0;
};
