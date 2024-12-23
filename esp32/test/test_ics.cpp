#include <Arduino.h>
#include <IcsHardSerialClass.h>

const byte EN_PIN = 2;
const long BAUDRATE = 115200;
const int TIMEOUT = 1000;

// Arduino IDEと同じようにデフォルトのSerialを使用
IcsHardSerialClass krs(&Serial, EN_PIN, BAUDRATE, TIMEOUT);

void setup() {
    krs.begin();
}

void loop() {
    int pos_i;
    int pos_f;
    
    pos_i = krs.degPos100(9000);
    krs.setPos(0, pos_i);
    delay(500);
    
    pos_f = krs.degPos(-90.0);
    krs.setPos(0, pos_f);
    delay(500);
}