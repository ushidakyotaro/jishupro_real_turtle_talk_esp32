#pragma once
#include <Arduino.h>
#include <WiFi.h>

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

struct SwimParameters {
    float periodSec;
    float wingDeg;
    float maxAngleDeg;
    float yRate;
    bool isBackward;
    SwimParameters() : periodSec(0), wingDeg(0), maxAngleDeg(0), yRate(0) {}
};

class MessageProcessor {
public:
    MessageProcessor();
    bool processMessage(WiFiClient& client);
    void statusResponse(WiFiClient& client);
    void sendResponse(WiFiClient& client, uint8_t response);
    
    CrushMode getCurrentMode() const { return currentMode; }
    WingUpMode getCurrentWingMode() const { return currentWingMode; }
    SwimParameters getCurrentParams() const { return currentParams; }
    bool getMouthOpen() const { return isMouthOpen; }

private:
    float bytesToFloat(const uint8_t* bytes);
    int16_t bytesToInt16(const uint8_t* bytes);
    
    SwimParameters currentParams;
    CrushMode currentMode;
    WingUpMode currentWingMode;
    bool isMouthOpen;
};