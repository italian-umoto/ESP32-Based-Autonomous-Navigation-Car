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
volatile CommandType pending_type = CommandType::NONE;
volatile bool command_available = false;
volatile uint32_t pending_value = 0;

const char* commandTypeToString(CommandType type) {
    switch (type) {
        case CommandType::NONE:      return "NONE";
        case CommandType::SET_STATE: return "SET_STATE";
        default:                     return "UNKNOWN";
    }
}

bool getCommand(Command& outCmd) {
    if (command_available) {
        outCmd.type = pending_type;
        outCmd.value = pending_value;
        command_available = false;
        return true;
    }
    return false;
}

static void parseAndStore(const String& message) {
    int eq = message.indexOf('=');

    String expectedPrefix = String(CLIENT_ID) + " set: STATE";

    if (message.startsWith(expectedPrefix) && eq != -1) {
        int32_t value = message.substring(eq + 1).toInt();

        pending_value = value;
        pending_type = CommandType::SET_STATE;
        command_available = true;
    }
    // can add other commands here with set: MOTION or something
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
