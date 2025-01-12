#pragma once
#include <Arduino.h>
#include <WiFi.h>

// ロボットの基本モード
enum class RobotMode {
    SERVO_OFF = 0,
    INIT_POSE = 1,
    MOTION_1 = 2,  // 基本モーション1
    MOTION_2 = 3,  // 基本モーション2
    MOTION_3 = 4,  // 基本モーション3
    EMERGENCY = 5
};

// モーション用パラメータ構造体
struct MotionParameters {
    float param1; 
    float param2; 
    float param3; 
    float param4;  
    bool  flag1;   // 方向やモードの切り替えフラグ
    
    MotionParameters() : param1(0), param2(0), param3(0), param4(0), flag1(false) {}
};

class MessageProcessor {
public:
    MessageProcessor();
    bool processMessage(WiFiClient& client);
    void statusResponse(WiFiClient& client);
    void sendResponse(WiFiClient& client, uint8_t response);
    
    RobotMode getCurrentMode() const { return currentMode; }
    MotionParameters getCurrentParams() const { return currentParams; }

private:
    float bytesToFloat(const uint8_t* bytes);
    int16_t bytesToInt16(const uint8_t* bytes);
    
    MotionParameters currentParams;
    RobotMode currentMode;
};