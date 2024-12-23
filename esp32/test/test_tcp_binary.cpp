// test_wifi_binary.cpp
#include <Arduino.h>
#include "wifi_connection.h"
#include "message_processor.h"

WiFiConnection wifiConnection;
MessageProcessor messageProcessor;

void setup() {
   Serial.begin(115200);
   
   if (wifiConnection.begin()) {
       Serial.println("Connected to WiFi");
       Serial.println(WiFi.localIP());
   }
}

void loop() {
   wifiConnection.handleConnection();
   
   if (wifiConnection.isConnected()) {
       WiFiClient client = wifiConnection.getServer()->available();
       
       if (client) {
           Serial.println("Client connected");
           wifiConnection.setClientConnected(true);
           
           while (client.connected()) {
                wifiConnection.handleConnection();
               if (messageProcessor.processMessage(client)) {
                   // メッセージ処理が成功した場合の処理
                   CrushMode mode = messageProcessor.getCurrentMode();
                   auto params = messageProcessor.getCurrentParams();
                   
                   Serial.printf("Mode: %d, Period: %.2f, Wing: %.1f, Max: %.1f, Y: %.2f\n",
                       static_cast<int>(mode),
                       params.periodSec,
                       params.wingDeg,
                       params.maxAngleDeg,
                       params.yRate);
               }
           }
           
           wifiConnection.setClientConnected(false);
           Serial.println("Client disconnected");
       }
   }
}
