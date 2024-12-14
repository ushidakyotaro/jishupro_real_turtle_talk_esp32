#include <WiFi.h>
#include "esp_wpa2.h"

#include "credentials.h"  // 認証情報をインクルード

// UTokyo WiFi credentials
const char *ssid = UTOKYO_SSID;
const char *username = UTOKYO_USERNAME;
const char *password = UTOKYO_PASSWORD;

#define LED_BUILTIN 2

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    Serial.println("\nConnecting to UTokyo WiFi...");
    
    // WiFiを初期化
    WiFi.disconnect(true);
    WiFi.mode(WIFI_STA);
    
    // WPA2 Enterprise設定
    esp_wifi_sta_wpa2_ent_set_identity((uint8_t *)username, strlen(username));
    esp_wifi_sta_wpa2_ent_set_username((uint8_t *)username, strlen(username));
    esp_wifi_sta_wpa2_ent_set_password((uint8_t *)password, strlen(password));
    esp_wifi_sta_wpa2_ent_enable();
    
    // WiFi接続開始
    WiFi.begin(ssid);
    
    // 接続待機
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 30) {
        delay(500);
        Serial.print(".");
        attempts++;
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\nConnected to UTokyo WiFi!");
        Serial.print("IP address: ");
        Serial.println(WiFi.localIP());
    } else {
        Serial.println("\nConnection failed!");
    }
    
    pinMode(LED_BUILTIN, OUTPUT);
}

void loop() {
    if (WiFi.status() == WL_CONNECTED) {
        // 接続中は早い点滅（200ms間隔）
        digitalWrite(LED_BUILTIN, HIGH);
        delay(200);
        digitalWrite(LED_BUILTIN, LOW);
        delay(200);
    } else {
        // 未接続時は遅い点滅（3秒間隔）
        digitalWrite(LED_BUILTIN, HIGH);
        delay(3000);
        digitalWrite(LED_BUILTIN, LOW);
        delay(3000);
        
        // 再接続を試みる
        Serial.println("Connection lost. Trying to reconnect...");
        WiFi.begin(ssid);
    }
}