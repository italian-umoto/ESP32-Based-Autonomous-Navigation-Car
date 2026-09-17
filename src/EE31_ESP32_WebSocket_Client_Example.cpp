#include <WiFi.h>
#include <WebSocketsClient.h>

#include <secrets.h>

// Network Configuration
// Update to tufts_eecs and network password
const char* WIFI_SSID = SECRET_SSID;
const char* WIFI_PASSWORD = SECRET_PASS;

const char* SERVER_IP = "10.5.9.24";  // IP of server ESP32
const uint16_t SERVER_PORT = 80;
const char* SERVER_PATH = "/ws";

const char* CLIENT_ID = "enter_your_id_here";

WebSocketsClient webSocket;

bool authenticated = false;
unsigned long lastSendTime = 0;

void webSocketEvent(
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

void setup() {
  Serial.begin(115200);

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
}

void loop() {
  webSocket.loop();

  if (authenticated &&
      millis() - lastSendTime >= 5000) {
    lastSendTime = millis();
    // what do you want to send?
    char* myMessage = "???";
    webSocket.sendTXT(
      myMessage
    );
  }
}
