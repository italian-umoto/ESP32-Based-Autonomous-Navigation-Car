#include <WiFi.h>
#include "websocket.h"
#include <websocket_secrets.h>

// SERVER CONSTANTS
//const char* SERVER_IP = "10.5.9.24"; // Tufts WS IP
const char* SERVER_IP = "10.0.0.95"; // AJ laptop IP
const uint16_t SERVER_PORT = 80;
const char* SERVER_PATH = "/ws";
const char* CLIENT_ID = "MAGICSMOKE67";

// NETWORK CONTFIG
const char* WIFI_SSID = SECRET_SSID;
const char* WIFI_PASSWORD = SECRET_PASS;

TaskHandle_t WebTask;
WebSocketsClient webSocket;
bool authenticated = false;
unsigned long lastSendTime = 0;


// These 3 *need* to be volatile for atomic writes?
//
// the only way we don't run into concurrency issues
// with two cores running is because the pending_value 
// write will happen atomically (i think)
volatile bool command_available = false;
volatile uint32_t pending_value = 0;
volatile uint32_t secondary_value = 0; // will be non-0 when in use for cmd

bool getCommand(Command& outCmd) {
    if (command_available) {
        outCmd.value = pending_value;
        outCmd.altValue = secondary_value;
        command_available = false;
        return true;
    }
    return false;
}

static bool parseInt32(const char *s, const char *end, int32_t *out) {
    char *stop;
    long v = strtol(s, &stop, 10);
    if (stop == s || stop != end) return false;
    *out = (int32_t)v;
    return true;
}

static void parseAndStore(const String& message) {
    const String prefix = String(CLIENT_ID) + " set: STATE=";
    if (!message.startsWith(prefix)) return;

    const char *body  = message.c_str() + prefix.length();
    const char *comma = strchr(body, ',');
    if (!comma) return;

    int32_t x, y;
    if (!parseInt32(body, comma, &x)) return;
    if (!parseInt32(comma + 1, body + strlen(body), &y)) return;

    pending_value     = x;
    secondary_value   = y;
    command_available = true;
}

// Extracted from starter code loop fn
// Is executed on separate core (core 0 - see websocket_init())
// I think webSocket.loop() handles the reconnect
static void websocketTask(void * pvParameters) {
    for (;;) {
        webSocket.loop();
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

static void webSocketEvent(
  WStype_t type,
  uint8_t* payload,
  size_t length
) {
  switch (type) {
    case WStype_CONNECTED:
      Serial.println("Connected to WebSocket server");

      // First message must be an approved ID!
      webSocket.sendTXT(CLIENT_ID);
      break;

    case WStype_TEXT: {
      String message;

      for (size_t i = 0; i < length; i++) {
        message += (char)payload[i];
      }

      Serial.print("Received: ");
      Serial.println(message);

      if (message.indexOf("\"authenticated\"") >= 0 &&
          message.indexOf("\"ok\"") >= 0) {
        authenticated = true;
        Serial.println("Client authenticated");
      }

      if (message.indexOf("\"error\"") >= 0) {
        authenticated = false;
        Serial.println("Authentication failed");
      }

      parseAndStore(message);
      break;
    }

    case WStype_DISCONNECTED:
      authenticated = false;
      Serial.println("Disconnected");
      break;

    case WStype_ERROR:
      Serial.println("WebSocket error");
      break;

    default:
      break;
  }
}

void websocketInit() {
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.println("Wi-Fi connected");

  webSocket.begin(
    SERVER_IP,
    SERVER_PORT,
    SERVER_PATH
  );

  webSocket.onEvent(webSocketEvent);
  webSocket.setReconnectInterval(5000);
  webSocket.enableHeartbeat(15000, 3000, 2);

  // Create an RTOS task to poll the websocket so we can FSM on other CPU
  // https://randomnerdtutorials.com/esp32-freertos-arduino-tasks
  xTaskCreatePinnedToCore(
    websocketTask,   // Function
    "WebTask",       // Name
    10000,           // Stack size
    NULL,            // Task input
    1,               // Priority
    &WebTask,        
    0                // Core #
  );
}
