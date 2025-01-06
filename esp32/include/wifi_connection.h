#pragma once
#include <Arduino.h>
#include <WiFi.h>
#include "esp_wpa2.h"
#include "credentials.h"

class WiFiConnection {
public:
    WiFiConnection();
    bool begin();
    bool isConnected();
    bool isClientConnected() const { return clientConnected; }
    void setClientConnected(bool connected) { clientConnected = connected; }
    void restartServer();
    bool reconnect();
    void handleConnection();
    WiFiServer* getServer() { return &server; }
    static constexpr int SERVER_PORT = 8000;

private:
    bool connectToWiFi(const char* ssid, bool isUTokyo = false);
    WiFiServer server;
    bool isUTokyoWiFi;
    bool clientConnected;
    bool tryAlternativeNetwork();
    static constexpr unsigned long RECONNECT_INTERVAL = 5000;
    //LED
    unsigned long lastReconnectAttempt;
    unsigned long lastLedToggle;
    bool ledState;
    bool isLongPhase;
    static constexpr int LED_BUILTIN = 2;  // LED_BUILTIN
    void updateLEDStatus(bool clientConnected);
};
