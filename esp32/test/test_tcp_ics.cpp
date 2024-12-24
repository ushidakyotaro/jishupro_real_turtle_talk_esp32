// test_tcp_ics.cpp
#include <Arduino.h>
#include "wifi_connection.h"
#include "message_processor.h"
#include <IcsHardSerialClass.h>

WiFiConnection wifiConnection;
MessageProcessor messageProcessor;

// サーボ設定
const byte EN_PIN = 2;
const long BAUDRATE = 115200;
const int TIMEOUT = 1000;
IcsHardSerialClass krs(&Serial, EN_PIN, BAUDRATE, TIMEOUT);

void setup() {
   Serial.begin(115200);
   krs.begin();
   
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
                    CrushMode mode = messageProcessor.getCurrentMode();
                    auto params = messageProcessor.getCurrentParams();
                   
                    Serial.printf("Mode: %d, Period: %.2f, Wing: %.1f, Max: %.1f, Y: %.2f\n",
                       static_cast<int>(mode),
                       params.periodSec,
                       params.wingDeg,
                       params.maxAngleDeg,
                       params.yRate);
                   
                   switch(mode) {
                       case CrushMode::SERVO_OFF:
                           // サーボオフ
                           break;
                           
                       case CrushMode::INIT_POSE:
                           // 角度0度に設定
                           krs.setPos(0, krs.degPos(0.0));

                           krs.setPos(1, krs.degPos(0.0));
                           break;
                           
                       default:
                           // 90度 → -90度の繰り返し
                           krs.setPos(0, krs.degPos100(9000));
                           krs.setPos(1, krs.degPos100(9000));
                           delay(500);
                           krs.setPos(0, krs.degPos(-90.0));
                           krs.setPos(1, krs.degPos(-90.0));
                           delay(500);

                        
                           break;
                   }
               }
           }
           
           wifiConnection.setClientConnected(false);
           Serial.println("Client disconnected");
       }
   }
}