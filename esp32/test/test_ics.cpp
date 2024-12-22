#include <Arduino.h>
#include <IcsHardSerialClass.h>

// ESP32のピン定義
const byte EN_PIN = 2;  // ENピンの番号は適宜変更
const long BAUDRATE = 115200;
const int TIMEOUT = 1000;

// ESP32ではHardwareSerial2を使用（他のシリアルポートも使用可能）
HardwareSerial SerialICS(2);  // UART2を使用
IcsHardSerialClass krs(&SerialICS, EN_PIN, BAUDRATE, TIMEOUT);

void setup() {
    // デバッグ用シリアル
    Serial.begin(115200);
    
    // サーボモータの通信初期設定
    krs.begin();
    
    Serial.println("Servo motor initialization completed");
}

void loop() {
    int pos_i;
    int pos_f;
    
    // 整数型(x100)の場合
    pos_i = krs.degPos100(9000);  // 90 x100deg をポジションデータに変換
    krs.setPos(0, pos_i);         // 変換したデータをID:0に送る
    Serial.println("Moving to 90 degrees");
    delay(500);
    
    // 浮動小数点の場合
    pos_f = krs.degPos(-90.0);    // -90.0deg(float)をポジションデータに変換
    krs.setPos(0, pos_f);         // 変換したデータをID:0に送る
    Serial.println("Moving to -90 degrees");
    delay(500);
}