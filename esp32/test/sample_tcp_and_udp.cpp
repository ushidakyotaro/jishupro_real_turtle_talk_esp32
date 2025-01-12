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
        // UDP: WiFi状態のブロードキャスト
        wifiConnection.broadcastStatus();
        
        // UDP: コマンドの受信処理
        WiFiUDP* udp = wifiConnection.getUDP();
        int packetSize = udp->parsePacket();
        
        if (packetSize) {
            uint8_t buffer[255];
            int len = udp->read(buffer, sizeof(buffer));
            
            if (len > 0) {
                if (messageProcessor.processUDPMessage(buffer, len)) {
                    udp->beginPacket(udp->remoteIP(), udp->remotePort());
                    uint8_t response = 0x00;
                    udp->write(&response, 1);
                    udp->endPacket();
                }
            }
        }

        // TCP: クライアント接続の処理
        WiFiClient client = wifiConnection.getServer()->available();
        
        if (client) {
            Serial.println("TCP Client connected");
            wifiConnection.setClientConnected(true);
            
            while (client.connected()) {
                wifiConnection.handleConnection();
                
                if (messageProcessor.processMessage(client)) {
                    CrushMode mode = messageProcessor.getCurrentMode();
                    auto params = messageProcessor.getCurrentParams();
                    
                    Serial.printf("TCP Mode: %d, Period: %.2f, Wing: %.1f, Max: %.1f, Y: %.2f\n",
                        static_cast<int>(mode),
                        params.periodSec,
                        params.wingDeg,
                        params.maxAngleDeg,
                        params.yRate);
                }
            }
            
            wifiConnection.setClientConnected(false);
            Serial.println("TCP Client disconnected");
        }
    }
}