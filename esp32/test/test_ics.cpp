#include <Arduino.h>
#include <IcsHardSerialClass.h>

const byte EN_PIN = 2;
const byte RX_PIN = 4;
const byte TX_PIN = 0;
//const long BAUDRATE = 115200;
const long BAUDRATE = 1250000;
const int TIMEOUT = 1000;

// Arduino IDEと同じようにデフォルトのSerialを使用
IcsHardSerialClass krs(&Serial, EN_PIN, BAUDRATE, TIMEOUT);

void setup() {
    Serial.begin(BAUDRATE, SERIAL_8E1, RX_PIN, TX_PIN, false, TIMEOUT);
    krs.begin();
}

void loop() {
    int pos_i;
    int pos_f;
    
  pos_i = krs.degPos(45);  //90 x100deg をポジションデータに変換
  for (int i = 0; i < 7; ++i) {
    krs.setPos(i, pos_i);//変換したデータをID:0に送る
  }
    delay(500);
    
  pos_i = krs.degPos(-45);  //90 x100deg をポジションデータに変換
  for (int i = 0; i < 7; ++i) {
    krs.setPos(i, pos_i);//変換したデータをID:0に送る
  }
    delay(500);
}