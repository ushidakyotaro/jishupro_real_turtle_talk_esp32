#include <Arduino.h>
#include <IcsHardSerialClass.h>

#include "motion_patterns.h"

// モードの定義
enum class CrushMode {
    SERVO_OFF = 0,
    INIT_POSE = 1,
    STAY = 2,
    SWIM = 3,
    RAISE = 4,
    EMERGENCY_SURFACE = 5  // 追加：WiFi切断時の水面浮上モード
};

enum class WingUpMode {
    RIGHT = 1,
    LEFT = 2,
    BOTH = 3
};

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
    // 既存のメソッドは維持
};
};


class CrushMain {
protected:
    CrushMode currentMode;
    CrushMode previousMode;
    double periodSec;
    double wingDeg;
    double maxAngleDeg;
    double yRate;
    WingUpMode wingUpMode;
    bool mouthOpen;
    ErrorStatus errorStatus;
    
    // 角度制限
    const double MAX_WING_ANGLE = 45.0;
    const double MIN_WING_ANGLE = -45.0;

    // WiFi関連
    const unsigned long WIFI_RECONNECT_INTERVAL = 5000; // ms
    const unsigned long MODE_TRANSITION_DELAY = 1000;   // ms
    bool isWifiConnected;
    unsigned long lastWifiCheckTime;

public:
    CrushMain() {
        initializeSystem();

    }
    
    virtual ~CrushMain() {
        setServoOff();
    }

    // モード変更時の処理
    void changeMode(CrushMode newMode) {
        if (newMode != currentMode) {
            previousMode = currentMode;
            // 一旦INIT_POSEに遷移
            if (newMode != CrushMode::INIT_POSE) {
                transitionToInitPose();
                delay(MODE_TRANSITION_DELAY);
            }
            currentMode = newMode;
        }
    }

protected:
    void initializeSystem() {
        initializeWiFi();
        initializeServos();
        currentMode = CrushMode::INIT_POSE;
        previousMode = CrushMode::SERVO_OFF;
    }

    // WiFi切断時の処理
    void handleWifiDisconnection() {
        errorStatus.wifiDisconnected = true;
        if (currentMode != CrushMode::EMERGENCY_SURFACE) {
            finishCurrentMovement();
            changeMode(CrushMode::EMERGENCY_SURFACE);
        }
    }

    // 角度チェック
    bool isAngleValid(double angle) {
        if (angle < MIN_WING_ANGLE || angle > MAX_WING_ANGLE) {
            errorStatus.angleOutOfRange = true;
            errorStatus.errorMessage = "Angle out of range: " + String(angle);
            // PCにエラーを送信
            sendErrorToPc();
            return false;
        }
        return true;
    }

    // 現在の動作を完了
    void finishCurrentMovement() {
        // 現在の周期が終わるまで待機
        while (!isMovementCycleComplete()) {
            delay(10);
        }
    }
    
    // 補間計算用
    SplineInterpolator splineInterpolator;
    std::vector<WingMotionPoint> currentPattern;
    double currentTimeRatio;

    // エラー送信
    void sendErrorToPc();
    void initializeWiFi();
    void initializeServos();
    void setServoOff();
    void transitionToInitPose();
    bool isMovementCycleComplete();
};

class CrushBody : public CrushMain {
private:
    unsigned long lastUpdateTime;
    unsigned long cycleStartTime;
    CycleTimer cycleTimer;
    double lastCalculatedAngle;
    
public:
    CrushBody() : lastUpdateTime(0), cycleStartTime(0) {}
    
    void mainLoop() {
        // WiFi接続チェック
        if (millis() - lastWifiCheckTime > WIFI_RECONNECT_INTERVAL) {
            checkWifiConnection();
        }

        // モードに応じた処理
        switch (currentMode) {
            case CrushMode::EMERGENCY_SURFACE:
                handleEmergencySurface();
                break;
            case CrushMode::SWIM:
                handleSwimMovement();
                break;
            // 他のモード処理...
        }
    }

private:
    void handleEmergencySurface() {
        // 水面浮上のための動作を実装
    }

    void handleSwimMovement() {
        if (isAngleValid(wingDeg)) {
            // 通常の遊泳動作
        } else {
            // エラー時は安全な角度で動作
            wingDeg = std::clamp(wingDeg, MIN_WING_ANGLE, MAX_WING_ANGLE);
        }
    }
};

class CrushMouth : public CrushMain {
    // 既存の実装を維持
};