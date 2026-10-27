#include "fsm.h"
#include "collision.h"
#include "pins.h"
#include "websocket.h"
#include <Arduino.h>

constexpr int32_t CMD_START_ROUTE = 7;
constexpr uint32_t BUTTON_DEBOUNCE_MS = 10;
constexpr uint8_t BRIGHT = 64;

// ---------------- LED stuff ------------------------
struct Rgb {
  uint8_t r, g, b;
};

constexpr Rgb STEP_COLORS[] = {
    {BRIGHT, 0, 0},      {0, BRIGHT, 0},      {0, 0, BRIGHT},
    {BRIGHT, BRIGHT, 0}, {BRIGHT, 0, BRIGHT}, {0, BRIGHT, BRIGHT},
};
constexpr uint8_t NUM_STEP_COLORS =
    sizeof(STEP_COLORS) / sizeof(STEP_COLORS[0]);

static void showRemote() {
  rgbLedWrite(STATE_INDICATOR_LED, BRIGHT / 4, BRIGHT / 4, BRIGHT / 4);
}

static void showStep(uint8_t idx) {
  const Rgb &c = STEP_COLORS[idx % NUM_STEP_COLORS];
  rgbLedWrite(STATE_INDICATOR_LED, c.r, c.g, c.b);
}

// -------------- FSM impl ----------------------------------

FSM::FSM() { pinMode(STATE_TRANSITION_PIN, INPUT_PULLUP); }

void FSM::tick() {
  // Set minimum period
  const uint32_t now = millis();
  if (now - m_lastTickMs < TICK_MS)
    return;
  m_lastTickMs = now;

  // Check command mailbox
  Command cmd;
  if (getCommand(cmd))
    handleCommand(cmd.value, cmd.altValue);
  else if (buttonPressed())
    startSolo(0);

  // Check if hitting a wall
  const bool wall = readCollisionDetection().imminent_collision;

  // Maybe we're trying to hit a wall
  if (wall && m_behavior.movesForward()) {
    const bool solo = m_mode == Mode::Solo;
    const bool exitCondIsWall = m_route.current().exit == Exit::Wall;
    if (!(solo && exitCondIsWall)) {
      Serial.println("Wall: stopping to avoid collision");
      m_behavior.start(Motion::Stop);
    }
  }

  // Actually do the motion
  const BehaviorStatus status = m_behavior.tick();

  // In Solo, let the route decide whether to move on
  if (m_mode == Mode::Solo) {
    // tick will return something that isn't nullptr when step is done
    const Step *next = m_route.tick(wall, status);
    if (next != nullptr)
      enterStep(*next);
  }
}

// Routes to starting solo mode at a specific step,
// or handles websocket command
void FSM::handleCommand(int32_t value, int32_t altValue) {
  if (value == CMD_START_ROUTE) {
    startSolo(static_cast<uint8_t>(altValue));
    return;
  }

  if (value < 0 || value > 6) {
    Serial.printf("Unknown command %ld\n", (long)value);
    return;
  }

  m_mode = Mode::Remote;
  Serial.printf("Remote: motion %ld, param %ld\n", (long)value, (long)altValue);
  m_behavior.start(static_cast<Motion>(value), static_cast<float>(altValue));
  showRemote();
}

void FSM::startSolo(uint8_t fromStep) {
  m_mode = Mode::Solo;
  enterStep(m_route.start(fromStep));
}

// Single place for transition side effects
void FSM::enterStep(const Step &step) {
  Serial.printf("Step %u: %s\n", m_route.index(), step.name);
  m_behavior.start(step.motion, step.param, step.color);
  showStep(m_route.index());
}

bool FSM::buttonPressed() {
  static int lastReading = HIGH;
  static int stableState = HIGH;
  static uint32_t lastChangeMs = 0;

  const int reading = digitalRead(STATE_TRANSITION_PIN);
  const uint32_t now = millis();

  if (reading != lastReading) {
    lastChangeMs = now;
    lastReading = reading;
  }

  if (now - lastChangeMs > BUTTON_DEBOUNCE_MS && reading != stableState) {
    stableState = reading;
    return stableState == LOW;
  }
  return false;
}
