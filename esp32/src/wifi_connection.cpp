//src/wifi_connection.cpp
#include "wifi_connection.h"

WiFiConnection::WiFiConnection() 
       : server(SERVER_PORT), isUTokyoWiFi(false), lastReconnectAttempt(0), 
     lastLedToggle(0), ledState(false), isLongPhase(true)  {
        pinMode(LED_BUILTIN, OUTPUT);
}

bool WiFiConnection::begin() {
    Serial.begin(115200);
    // UTokyo WiFiを先に試行
    if (connectToWiFi(UTOKYO_SSID, true)) {
        isUTokyoWiFi = true;
        server.begin();
        return true;
    // 次にホームWiFiを試行
    } else if (connectToWiFi(HOME_SSID)) {
        isUTokyoWiFi = false;
        server.begin();
        return true;
    }
    return false;
}

bool WiFiConnection::connectToWiFi(const char* ssid, bool isUTokyo) {
    Serial.print("Attempting to connect to: ");
    Serial.println(ssid);
    if (isUTokyo) {
        WiFi.disconnect(true);
        WiFi.mode(WIFI_STA);
        
        esp_wifi_sta_wpa2_ent_set_identity((uint8_t *)UTOKYO_USERNAME, strlen(UTOKYO_USERNAME));
        esp_wifi_sta_wpa2_ent_set_username((uint8_t *)UTOKYO_USERNAME, strlen(UTOKYO_USERNAME));
        esp_wifi_sta_wpa2_ent_set_password((uint8_t *)UTOKYO_PASSWORD, strlen(UTOKYO_PASSWORD));
        esp_wifi_sta_wpa2_ent_enable();
        
        WiFi.begin(ssid);
    } else {
        WiFi.begin(ssid, HOME_PASSWORD);
    }
    
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 30) {
        delay(500);
        attempts++;
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("WiFi Connected Successfully!");
        Serial.print("Connected to SSID: ");
        Serial.println(ssid);
        Serial.print("IP Address: ");
        Serial.println(WiFi.localIP());
        return true;
    }
    return false;
}

bool WiFiConnection::tryAlternativeNetwork() {
    isUTokyoWiFi = !isUTokyoWiFi;
    return connectToWiFi(isUTokyoWiFi ? UTOKYO_SSID : HOME_SSID, isUTokyoWiFi);
}

void WiFiConnection::restartServer() {
    server.begin();
}



bool WiFiConnection::isConnected() {
    return WiFi.status() == WL_CONNECTED;
}

bool WiFiConnection::reconnect() {
    if (millis() - lastReconnectAttempt < RECONNECT_INTERVAL) {
        return false;
    }
    
    lastReconnectAttempt = millis();
    
    // 現在のネットワークに再接続を試みる
    if (connectToWiFi(isUTokyoWiFi ? UTOKYO_SSID : HOME_SSID, isUTokyoWiFi)) {
        restartServer();
        return true;
    }
    
    // 失敗したら別のネットワークを試す
    Serial.println("Trying alternative network...");
    if (tryAlternativeNetwork()) {
        restartServer();
        return true;
    }
    
    Serial.println("Reconnection failed");
    return false;
}


void WiFiConnection::handleConnection() {
    if (!isConnected()) {
        clientConnected = false;
        reconnect();
    }
    updateLEDStatus(clientConnected);
}

void WiFiConnection::updateLEDStatus(bool clientConnected) {
   unsigned long currentMillis = millis();
   
   if (!isConnected()) {
       // WiFi未接続: 3秒間隔
       if (currentMillis - lastLedToggle >= 3000) {
           ledState = !ledState;
           lastLedToggle = currentMillis;
       }
   } else if (clientConnected) {
       // クライアント接続中: 100ms間隔
       if (currentMillis - lastLedToggle >= 200) {  // 合計周期を200msに設定
           ledState = !ledState;
           lastLedToggle = currentMillis;
       }
   } else {
       // WiFi接続中: ON 2秒, OFF 0.5秒
       unsigned long interval = ledState ? 2000 : 500;  // HIGHなら2秒、LOWなら0.5秒
       if (currentMillis - lastLedToggle >= interval) {
           ledState = !ledState;
           lastLedToggle = currentMillis;
       }
   }
   digitalWrite(LED_BUILTIN, ledState);
}