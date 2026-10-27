#pragma once
#include "behavior.h"
#include "route.h"
#include <stdint.h>

/* Mode
 *
 * Who is choosing the Motion
 */
enum class Mode {
  Remote,
  Solo,
};

/* FSM
 *
 * Owns: the command mailbox, mode, wall read, and tick rate
 *
 * Commands (cmd.value, cmd.altValue):
 *   0           -> Remote, stop
 *   1..6, param -> Remote, that Motion with altValue as its param
 *   7, step     -> Solo, start the route at `step` (0 = full run)
 * Button        -> Solo, start the route at step 0
 */
class FSM {
public:
  FSM();

  // Call from loop() in main.cpp
  void tick();

private:
  void handleCommand(int32_t value, int32_t altValue);
  void startSolo(uint8_t fromStep);
  void enterStep(const Step &step);
  bool buttonPressed();

  static constexpr uint32_t TICK_MS = 10;

  Behavior m_behavior;
  Route m_route;
  Mode m_mode = Mode::Remote;
  uint32_t m_lastTickMs = 0;
};
