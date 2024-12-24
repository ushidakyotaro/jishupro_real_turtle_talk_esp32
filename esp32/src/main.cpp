#include <Arduino.h>
#include <IcsHardSerialClass.h>
#include "wifi_connection.h"
#include "message_processor.h"
#include "motion_patterns.h"

// サーボ設定
const byte EN_PIN = 2;
const long BAUDRATE = 115200;
const int TIMEOUT = 1000;

// エラーステータス
struct ErrorStatus {
    bool wifiDisconnected = false;
    bool servoError = false;
    bool angleOutOfRange = false;
    String errorMessage;
};

struct CycleTimer {
    unsigned long startTime;
    double periodSec;
    
    double getCurrentTimeRatio() {
        return fmod((millis() - startTime) / (periodSec * 1000.0), 1.0);
    }
    
    void reset() {
        startTime = millis();
    }
    
    void start(double period) {
        periodSec = period;
        reset();
    }
    
    bool isCycleComplete() {
        return getCurrentTimeRatio() >= 1.0;
    }
};

class CrushMain {
protected:
    static IcsHardSerialClass krs;
    static WiFiConnection wifiConnection;
    static MessageProcessor messageProcessor;
    
    CrushMode currentMode;
    CrushMode previousMode;
    ErrorStatus errorStatus;
    
    const double MAX_WING_ANGLE = 45.0;
    const double MIN_WING_ANGLE = -45.0;
    const unsigned long WIFI_RECONNECT_INTERVAL = 5000;
    const unsigned long MODE_TRANSITION_DELAY = 1000;
    
    virtual void updateMotion() = 0;

public:
    CrushMain() : currentMode(CrushMode::INIT_POSE), 
                  previousMode(CrushMode::SERVO_OFF) {}
    
    virtual ~CrushMain() {
        setServoOff();
    }

    static void initializeSystem() {
        Serial.begin(115200);
        krs.begin();
        
        if (wifiConnection.begin()) {
            Serial.println("WiFi Connected");
            Serial.println(WiFi.localIP());
        }
    }

    virtual void loop() {
        if (wifiConnection.isConnected()) {
            WiFiClient client = wifiConnection.getServer()->available();
            if (client && client.connected()) {
                messageProcessor.processMessage(client);
            }
            updateMotion();
        } else {
            handleWifiDisconnection();
        }
    }

protected:
    void setServoOff() {
        krs.setFree(0);
    }

    bool isAngleValid(double angle) {
        if (angle < MIN_WING_ANGLE || angle > MAX_WING_ANGLE) {
            errorStatus.angleOutOfRange = true;
            errorStatus.errorMessage = "Angle out of range: " + String(angle);
            return false;
        }
        return true;
    }

    void handleWifiDisconnection() {
        if (!errorStatus.wifiDisconnected) {
            errorStatus.wifiDisconnected = true;
            if (currentMode != CrushMode::EMERGENCY_SURFACE) {
                currentMode = CrushMode::EMERGENCY_SURFACE;
            }
        }
    }
};

// 静的メンバの定義
IcsHardSerialClass CrushMain::krs(&Serial, EN_PIN, BAUDRATE, TIMEOUT);
WiFiConnection CrushMain::wifiConnection;
MessageProcessor CrushMain::messageProcessor;

class CrushBody : public CrushMain {
private:
    SplineInterpolator splineInterpolator;
    std::vector<WingMotionPoint> currentPattern;
    CycleTimer cycleTimer;
    const int BODY_SERVO_ID = 0;
    unsigned long lastUpdateTime = 0;

protected:
    void updateMotion() override {
        auto mode = messageProcessor.getCurrentMode();
        auto params = messageProcessor.getCurrentParams();
        
        switch (mode) {
            case CrushMode::SERVO_OFF:
                setServoOff();
                break;
            case CrushMode::INIT_POSE:
                krs.setPos(BODY_SERVO_ID, krs.degPos(0.0));
                break;
            case CrushMode::STAY:
                handleStayMode(params);
                break;
            case CrushMode::SWIM:
                handleSwimMode(params);
                break;
            case CrushMode::RAISE:
                handleRaiseMode(params);
                break;
            case CrushMode::EMERGENCY_SURFACE:
                handleEmergencySurface();
                break;
        }
    }

private:
    void handleStayMode(const SwimParameters& params) {
    if (currentPattern.empty() || currentPattern[0].timeRatio != MotionPatterns::STAY_PATTERN[0].timeRatio) {
        currentPattern = MotionPatterns::STAY_PATTERN;
        splineInterpolator.calculateCoefficients(currentPattern);
    }

        double timeRatio = cycleTimer.getCurrentTimeRatio();
        double angle = splineInterpolator.interpolate(timeRatio, currentPattern) * params.wingDeg;
        if (isAngleValid(angle)) {
            krs.setPos(BODY_SERVO_ID, krs.degPos(angle));
        }
    }

    void handleSwimMode(const SwimParameters& params) {
        if (currentPattern != MotionPatterns::SWIM_PATTERN) {
            currentPattern = MotionPatterns::SWIM_PATTERN;
            splineInterpolator.calculateCoefficients(currentPattern);
          }
        cycleTimer.start(params.periodSec);
        
        double timeRatio = cycleTimer.getCurrentTimeRatio();
        double baseAngle = splineInterpolator.interpolate(timeRatio, currentPattern) * params.maxAngleDeg;
        double finalAngle = baseAngle + params.yRate * params.wingDeg;
        
        if (isAngleValid(finalAngle)) {
            krs.setPos(BODY_SERVO_ID, krs.degPos(finalAngle));
        }
    }

    void handleRaiseMode(const SwimParameters& params) {
        WingUpMode mode = messageProcessor.getCurrentWingMode();
        double angle = params.wingDeg;
        if (isAngleValid(angle)) {
            switch (mode) {
                case WingUpMode::RIGHT:
                    krs.setPos(BODY_SERVO_ID, krs.degPos(angle));
                    break;
                case WingUpMode::LEFT:
                    krs.setPos(BODY_SERVO_ID, krs.degPos(-angle));
                    break;
                case WingUpMode::BOTH:
                    krs.setPos(BODY_SERVO_ID, krs.degPos(angle));
                    break;
            }
        }
    }

    void handleEmergencySurface() {
        double surfaceAngle = 30.0;
        krs.setPos(BODY_SERVO_ID, krs.degPos(surfaceAngle));
    }
};

class CrushMouth : public CrushMain {
private:
    const int MOUTH_SERVO_ID = 1;
    const double MOUTH_OPEN_ANGLE = 30.0;
    const double MOUTH_CLOSE_ANGLE = 0.0;
    unsigned long lastMouthUpdate = 0;
    const unsigned long MOUTH_UPDATE_INTERVAL = 100;

protected:
    void updateMotion() override {
        unsigned long currentTime = millis();
        if (currentTime - lastMouthUpdate >= MOUTH_UPDATE_INTERVAL) {
            bool shouldOpen = messageProcessor.getMouthOpen();
            double targetAngle = shouldOpen ? MOUTH_OPEN_ANGLE : MOUTH_CLOSE_ANGLE;
            krs.setPos(MOUTH_SERVO_ID, krs.degPos(targetAngle));
            lastMouthUpdate = currentTime;
        }
    }
};

// グローバルインスタンス
CrushBody body;
CrushMouth mouth;

void setup() {
    CrushMain::initializeSystem();
}

void loop() {
    body.loop();
    mouth.loop();
}