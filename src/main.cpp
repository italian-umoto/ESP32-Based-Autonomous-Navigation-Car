#include <WiFi.h>
#include <WebSocketsClient.h>
#include "websocket.h"

Command command;


void setup() {
    Serial.begin(115200);
    websocketInit();
}

void loop() {
    bool ret = getCommand(command);
    if (ret) {
        Serial.println("New Command:");
        Serial.print("type: ");
        Serial.println(commandTypeToString(command.type));
        Serial.print("value: ");
        Serial.println(command.value);
    }
    delay(1000); 
}

