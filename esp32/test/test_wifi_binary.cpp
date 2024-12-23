#include <WiFi.h>
#include "esp_wpa2.h"
#include "credentials.h"

// WiFi設定
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

// モードの定義
enum class CrushMode {
    SERVO_OFF = 0,
    INIT_POSE = 1,
    STAY = 2,
    SWIM = 3,
    RAISE = 4,
    EMERGENCY_SURFACE = 5
};

enum class WingUpMode {
    RIGHT = 1,
    LEFT = 2,
    BOTH = 3
};

// パラメータを保持する構造体
struct SwimParameters {
    float periodSec;
    float wingDeg;
    float maxAngleDeg;
    float yRate;
    SwimParameters() : periodSec(0), wingDeg(0), maxAngleDeg(0), yRate(0) {}
};

// グローバル変数
SwimParameters currentParams;
CrushMode currentMode = CrushMode::INIT_POSE;
WingUpMode currentWingMode = WingUpMode::BOTH;
bool mouthOpen = false;

// データ変換用関数
float bytesToFloat(const uint8_t* bytes) {
    float value;
    memcpy(&value, bytes, 4);
    return value;
}

int16_t bytesToInt16(const uint8_t* bytes) {
    return (bytes[1] << 8) | bytes[0];
}

// レスポンス送信用関数
void sendResponse(WiFiClient& client, uint8_t response) {
    client.write(&response, 1);
}

// メッセージ処理関数
bool processMessage(WiFiClient& client) {
    if (!client.available()) return true;

    uint8_t commandByte = client.read();
    uint8_t commandType = (commandByte >> 4) & 0x0F;
    uint8_t subCommand = commandByte & 0x0F;

    switch (commandType) {
        case 0x01: { // モード設定
            if (subCommand <= 5) {
                currentMode = static_cast<CrushMode>(subCommand);
                Serial.printf("Mode changed to: %d\n", static_cast<int>(currentMode));
                sendResponse(client, 0x00);
            } else {
                sendResponse(client, 0xE1);
            }
            break;
        }

        case 0x02: { // 遊泳パラメータ設定
            if (client.available() >= 9) {
                uint8_t buffer[9];
                size_t bytesRead = client.readBytes(buffer, 9);
                if (bytesRead != 9) {
                    sendResponse(client, 0xE3);  // 読み取りエラー
                    break;
                }

                float period = bytesToFloat(buffer);
                float wingDeg = bytesToInt16(buffer + 4) / 10.0f;
                float maxAngle = bytesToInt16(buffer + 6) / 10.0f;
                float yRate = static_cast<int8_t>(buffer[8]) / 100.0f;

                // 範囲チェック
                if (wingDeg >= -45.0 && wingDeg <= 45.0 &&
                    maxAngle >= -45.0 && maxAngle <= 45.0 &&
                    yRate >= -1.0 && yRate <= 1.0 &&
                    period > 0) {
                    
                    currentParams.periodSec = period;
                    currentParams.wingDeg = wingDeg;
                    currentParams.maxAngleDeg = maxAngle;
                    currentParams.yRate = yRate;

                    Serial.printf("Swim parameters: period=%.2f, wing=%.1f, max=%.1f, y=%.2f\n",
                        period, wingDeg, maxAngle, yRate);
                    sendResponse(client, 0x00);
                } else {
                    sendResponse(client, 0xE2);
                }
            } else {
                sendResponse(client, 0xE3);
            }
            break;
        }

        case 0x03: { // 翼制御
            if (client.available() >= 2) {
                uint8_t angleBytes[2];
                size_t bytesRead = client.readBytes(angleBytes, 2);
                if (bytesRead != 2) {
                    sendResponse(client, 0xE3);
                    break;
                }

                float angle = bytesToInt16(angleBytes) / 10.0f;

                if (angle >= -45.0 && angle <= 45.0 && subCommand >= 1 && subCommand <= 3) {
                    currentWingMode = static_cast<WingUpMode>(subCommand);
                    Serial.printf("Wing control: mode=%d, angle=%.1f\n",
                        static_cast<int>(currentWingMode), angle);
                    sendResponse(client, 0x00);
                } else {
                    sendResponse(client, 0xE2);
                }
            } else {
                sendResponse(client, 0xE3);
            }
            break;
        }

        case 0x04: { // 口制御
            mouthOpen = (subCommand & 0x01) != 0;
            Serial.printf("Mouth: %s\n", mouthOpen ? "open" : "closed");
            sendResponse(client, 0x00);
            break;
        }

        case 0x0F: { // ステータス要求
            uint8_t response[8] = {0};
            response[0] = static_cast<uint8_t>(currentMode);
            int16_t currentAngle = static_cast<int16_t>(currentParams.wingDeg * 10);
            memcpy(response + 1, &currentAngle, 2);
            client.write(response, sizeof(response));
            break;
        }

        default:
            sendResponse(client, 0xE0);
            break;
    }
    return true;
}

// WiFi接続関数
bool connectToWiFi(const char* ssid, bool isUTokyo = false) {
    Serial.print("\nAttempting to connect to: ");
    Serial.println(ssid);
    
    if (isUTokyo) {
        WiFi.disconnect(true);
        WiFi.mode(WIFI_STA);
        
        esp_wifi_sta_wpa2_ent_set_identity((uint8_t *)utokyo_username, strlen(utokyo_username));
        esp_wifi_sta_wpa2_ent_set_username((uint8_t *)utokyo_username, strlen(utokyo_username));
        esp_wifi_sta_wpa2_ent_set_password((uint8_t *)utokyo_password, strlen(utokyo_password));
        esp_wifi_sta_wpa2_ent_enable();
        
        WiFi.begin(ssid);
    } else {
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
        
        if (client) {
            Serial.println("New client connected!");
            clientConnected = true;
            
            while (client.connected()) {
                // メッセージ処理
                if (!processMessage(client)) {
                    break;
                }
                
                // クライアント接続中は早い点滅
                digitalWrite(LED_BUILTIN, HIGH);
                delay(100);
                digitalWrite(LED_BUILTIN, LOW);
                delay(100);
            }
            
            clientConnected = false;
            Serial.println("Client disconnected");
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