#include <WiFi.h>
#include "esp_wpa2.h"

#include "credentials.h"  // 認証情報をインクルード

const char *home_ssid = HOME_SSID;
const char *home_password = HOME_PASSWORD;
const char *utokyo_ssid = UTOKYO_SSID;
const char *utokyo_username = UTOKYO_USERNAME;
const char *utokyo_password = UTOKYO_PASSWORD;

// TCP server port
WiFiServer server(8000);
#define LED_BUILTIN 2

// 接続状態を追跡
bool clientConnected = false;
bool isUTokyoWiFi = false;

// WiFi接続を試みる関数
bool connectToWiFi(const char* ssid, bool isUTokyo = false) {
    Serial.print("\nAttempting to connect to: ");
    Serial.println(ssid);
    
    if (isUTokyo) {
        // UTokyo WiFiの場合
        WiFi.disconnect(true);
        WiFi.mode(WIFI_STA);
        
        esp_wifi_sta_wpa2_ent_set_identity((uint8_t *)utokyo_username, strlen(utokyo_username));
        esp_wifi_sta_wpa2_ent_set_username((uint8_t *)utokyo_username, strlen(utokyo_username));
        esp_wifi_sta_wpa2_ent_set_password((uint8_t *)utokyo_password, strlen(utokyo_password));
        esp_wifi_sta_wpa2_ent_enable();
        
        WiFi.begin(ssid);
    } else {
        // 家のWiFiの場合
        WiFi.begin(ssid, home_password);
    }
    
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 30) {
        delay(500);
        Serial.print(".");
        attempts++;
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\nConnected successfully!");
        Serial.print("IP address: ");
        Serial.println(WiFi.localIP());
        return true;
    }
    
    Serial.println("\nConnection failed!");
    return false;
}

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    pinMode(LED_BUILTIN, OUTPUT);
    
    // まず家のWiFiに接続を試みる
    if (connectToWiFi(home_ssid)) {
        isUTokyoWiFi = false;
    } else {
        // 家のWiFiに接続できない場合、UTokyo WiFiを試す
        if (connectToWiFi(utokyo_ssid, true)) {
            isUTokyoWiFi = true;
        }
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        // TCPサーバーを開始
        server.begin();
        Serial.println("TCP Server started");
    }
}

void loop() {
    // WiFi接続が切れた場合
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("WiFi connection lost. Attempting to reconnect...");
        clientConnected = false;
        
        // 現在のネットワークに再接続を試みる
        bool reconnected = connectToWiFi(isUTokyoWiFi ? utokyo_ssid : home_ssid, isUTokyoWiFi);
        
        // 再接続に失敗した場合、別のネットワークを試す
        if (!reconnected) {
            isUTokyoWiFi = !isUTokyoWiFi;
            connectToWiFi(isUTokyoWiFi ? utokyo_ssid : home_ssid, isUTokyoWiFi);
        }
        
        // 接続できた場合、TCPサーバーを再起動
        if (WiFi.status() == WL_CONNECTED) {
            server.begin();
        }
    }
    
    // WiFi接続中の処理
    if (WiFi.status() == WL_CONNECTED) {
        WiFiClient client = server.available();
        
        if (client) {  // クライアント接続時
            Serial.println("New client connected!");
            clientConnected = true;
            
            while (client.connected()) {
                if (client.available()) {
                    String request = client.readStringUntil('\r');
                    Serial.println("Received: " + request);
                    client.println("ESP32 received: " + request);
                    
                    if (request.indexOf("quit") != -1) {
                        client.stop();
                        clientConnected = false;
                        Serial.println("Client disconnected by command");
                        break;
                    }
                }
                
                // クライアント接続中は早い点滅
                digitalWrite(LED_BUILTIN, HIGH);
                delay(100);
                digitalWrite(LED_BUILTIN, LOW);
                delay(100);
            }
        } else {
            // WiFi接続中だがクライアント未接続は普通の速さで点滅
            digitalWrite(LED_BUILTIN, HIGH);
            delay(2000);
            digitalWrite(LED_BUILTIN, LOW);
            delay(500);
        }
    } else {
        // WiFi未接続時は遅い点滅
        digitalWrite(LED_BUILTIN, HIGH);
        delay(3000);
        digitalWrite(LED_BUILTIN, LOW);
        delay(3000);
    }
}