// test_tcp_ics.cpp
#include <Arduino.h>
#include "wifi_connection.h"
#include "message_processor.h"
#include <IcsHardSerialClass.h>

WiFiConnection wifiConnection;
MessageProcessor messageProcessor;

// サーボ設定
const byte EN_PIN = 5;
const byte RX_PIN = 16;
const byte TX_PIN = 17;
//const long BAUDRATE = 115200;
const long BAUDRATE = 1250000;
const int TIMEOUT = 20;

IcsHardSerialClass krs(&Serial2, EN_PIN, BAUDRATE, TIMEOUT);

void setup() {
   Serial2.begin(BAUDRATE, SERIAL_8E1, RX_PIN, TX_PIN, false, TIMEOUT);
   krs.begin();
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
            //if (client.connected()) {// 変更

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

                        int pos;

                       case CrushMode::SERVO_OFF:
                            for (int i = 0; i < 7; ++i) {
                                krs.setFree(i);//変換したデータをID:0に送る
                            }
                           // サーボオフ

                           break;
                           
                       case CrushMode::INIT_POSE:
                           // 角度0度に設定
                           
                        pos = krs.degPos(0);  
                        for (int i = 0; i < 7; ++i) {
                            krs.setPos(i, pos);//変換したデータをID:0に送る
                        }
                           break;
                           
                       default:
                            int pos_i;
                            int pos_f;
                            
                        pos_i = krs.degPos(30);  //90 x100deg をポジションデータに変換
                        for (int i = 0; i < 5; ++i) {
                            while (krs.setPos(i, pos_i) == -1) {
                                delay(1);
                            }
                        }
                            delay(500);
                            
                        pos_i = krs.degPos(-30);  //90 x100deg をポジションデータに変換
                        for (int i = 0; i < 5; ++i) {
                                               while (krs.setPos(i, pos_i) == -1) {
                                delay(1);
                            }
                        }
                            delay(2000);
                           break;
                   }
               }
           }
           
           wifiConnection.setClientConnected(false);
           Serial.println("Client disconnected");
       }
   }
}
