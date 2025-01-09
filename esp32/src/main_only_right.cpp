#include <Arduino.h>
#include <IcsHardSerialClass.h>
#include "wifi_connection.h"
#include "message_processor.h"
#include "motion_patterns.h"

// サーボ設定
const byte EN_PIN = 16;
const byte RX_PIN = 17;
const byte TX_PIN = 5;
const long BAUDRATE = 1250000;
const int TIMEOUT = 1000;

const int SERVO_NUM = 4; //右のみ

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
        for (int i = 0; i < SERVO_NUM; ++i) {
            krs.setFree(i);//変換したデータをID:0に送る
        }
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

    //parameters
    // クラス内でパラメータを保持
    const int BASE_SPEED = 60;
    const int SPEED_VARIATION = 40;
    
    // サーボIDの定義を明確に
    const int RIGHT_UP_DOWN_ID = 1;
    const int RIGHT_FRONT_BACK_ID = 2;
    const int LEFT_UP_DOWN_ID = 4;
    const int LEFT_FRONT_BACK_ID = 5;    

    const double DEFAULT_SURFACE_ANGLE = 30.0;
    const double WING_ROTATION_ANGLE = 90.0;


protected:
    void updateMotion() override {
        auto mode = messageProcessor.getCurrentMode();
        auto params = messageProcessor.getCurrentParams();
        
        switch (mode) {
            case CrushMode::SERVO_OFF:
                setServoOff();
                break;
            case CrushMode::INIT_POSE:
                handleInitMode();
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
    void sendVec2ServoPos(int posVec[SERVO_NUM], int speedVec[SERVO_NUM]){
        static int defaultSpeed[SERVO_NUM] = {127, 127, 127, 127, 127, 127, 127};
        if (speedVec == nullptr) {
            speedVec = defaultSpeed;
        }
        for (int i = 0; i < SERVO_NUM; ++i) {
            if (isAngleValid(posVec[i])) {
                while (krs.setSpd(i,speedVec[i]) == -1) {
                    delay(1);
                }
                while (krs.setPos(i, posVec[i]) == -1) {
                    delay(1);
                }
            }
        }
    }

    void handleInitMode() {
        // if (currentMode == CrushMode::INIT_POSE) {
            int pos = 0;
            for (int i = 0; i < SERVO_NUM; ++i) {
                while (krs.setPos(i, pos) == -1) {
                    delay(1);
                }
            }
        }
        // }

    // void handleStayMode(const SwimParameters& params) {
    // if (currentPattern.empty() || currentPattern[0].timeRatio != MotionPatterns::STAY_PATTERN[0].timeRatio) {
    //     currentPattern = MotionPatterns::STAY_PATTERN;
    //     splineInterpolator.calculateCoefficients(currentPattern);
    // }

    //     double timeRatio = cycleTimer.getCurrentTimeRatio();
    //     double angle = splineInterpolator.interpolate(timeRatio, currentPattern) * params.wingDeg;
    //     if (isAngleValid(angle)) {
    //         krs.setPos(BODY_SERVO_ID, krs.degPos(angle));
    //     }
    // }

    void handleStayMode(const SwimParameters& params) {
            // モード遷移の処理
        if (previousMode != CrushMode::STAY) {
            cycleTimer.reset();
            previousMode = CrushMode::STAY;
        }

        // 周期の更新確認
        if (cycleTimer.periodSec != params.periodSec) {
            cycleTimer.start(params.periodSec);
        }
        
        double timeRatio = cycleTimer.getCurrentTimeRatio();

        //wingdeg 2 wingRad 
        double wingRad = params.wingDeg * PI / 180.0;
        
        // calculate the angles of the wings
        double angle1 = params.maxAngleDeg * sin(wingRad) * sin(TWO_PI * timeRatio);
        double angle2 = params.maxAngleDeg * cos(wingRad) * sin(TWO_PI * timeRatio);

        int positions[SERVO_NUM] = {0, 0, 0, 0}; //右のみ
        int speeds[SERVO_NUM] = {127, 127, 127, 127}; //右のみ

            // 速度計算
        double speedFactor = abs(cos(TWO_PI * timeRatio));
        int currentSpeed = BASE_SPEED + (int)(SPEED_VARIATION * speedFactor);
        
        // 右腕の設定
        positions[RIGHT_UP_DOWN_ID] = krs.degPos(angle1);
        positions[RIGHT_FRONT_BACK_ID] = krs.degPos(angle2);
        speeds[RIGHT_UP_DOWN_ID] = currentSpeed;
        speeds[RIGHT_FRONT_BACK_ID] = currentSpeed;
        
        // 左腕の設定（右腕と同期）
        positions[LEFT_UP_DOWN_ID] = krs.degPos(angle1);
        positions[LEFT_FRONT_BACK_ID] = krs.degPos(angle2);
        speeds[LEFT_UP_DOWN_ID] = currentSpeed;
        speeds[LEFT_FRONT_BACK_ID] = currentSpeed;
        
        // 位置と速度を設定
        sendVec2ServoPos(positions, speeds);
        }
    

    // void handleSwimMode(const SwimParameters& params) {
    //     if (currentPattern != MotionPatterns::SWIM_PATTERN) {
    //         currentPattern = MotionPatterns::SWIM_PATTERN;
    //         splineInterpolator.calculateCoefficients(currentPattern);
    //       }
    //     cycleTimer.start(params.periodSec);
        
    //     double timeRatio = cycleTimer.getCurrentTimeRatio();
    //     double baseAngle = splineInterpolator.interpolate(timeRatio, currentPattern) * params.maxAngleDeg;
    //     double finalAngle = baseAngle + params.yRate * params.wingDeg;
        
    //     if (isAngleValid(finalAngle)) {
    //         krs.setPos(BODY_SERVO_ID, krs.degPos(finalAngle));
    //     }
    // }
    
    void handleSwimMode(const SwimParameters& params) {
    // パラメータの妥当性チェック


    // モード遷移の処理
    if (previousMode != CrushMode::SWIM) {
        cycleTimer.reset();
        previousMode = CrushMode::SWIM;
    }

    // 周期の更新確認
    if (cycleTimer.periodSec != params.periodSec) {
        cycleTimer.start(params.periodSec);
    }

    double timeRatio = cycleTimer.getCurrentTimeRatio();
    double wingRad = params.wingDeg * PI / 180.0;
    
    // 左右の振幅調整（yRateに基づく）
    double rightAmplitude = params.maxAngleDeg * (1.0 + params.yRate) / 2.0;
    double leftAmplitude = params.maxAngleDeg * (1.0 - params.yRate) / 2.0;
    
    // 基本の角度計算
    double baseAngle1 = sin(wingRad) * sin(TWO_PI * timeRatio);  // 上下運動
    double baseAngle2 = cos(wingRad) * sin(TWO_PI * timeRatio);  // 前後運動
    
    // 3番と6番サーボの角度計算（前進/後退用）
    double angle3 = 0.0;
    if (!params.isBackward) {  // 前進の場合
        // baseAngle2が正から負に変化する領域（前から後ろに動かすとき）で90度
        if (cos(TWO_PI * timeRatio) < 0) {
            angle3 = WING_ROTATION_ANGLE;
        }
    } else {  // 後退の場合
        // baseAngle2が負から正に変化する領域（後ろから前に動かすとき）で90度
        if (cos(TWO_PI * timeRatio) > 0) {
            angle3 = WING_ROTATION_ANGLE;
        }
    }

    int positions[SERVO_NUM] = {0};
    int speeds[SERVO_NUM] = {0};
    
    // 速度計算
    double speedFactor = abs(cos(TWO_PI * timeRatio));
    int currentSpeed = BASE_SPEED + (int)(SPEED_VARIATION * speedFactor);
    
    try {
        // 右腕の設定
        positions[RIGHT_UP_DOWN_ID] = krs.degPos(rightAmplitude * baseAngle1);
        positions[RIGHT_FRONT_BACK_ID] = krs.degPos(rightAmplitude * baseAngle2);
        positions[3] = krs.degPos(angle3);  // 右の傾き制御
        
        // 左腕の設定
        positions[LEFT_UP_DOWN_ID] = krs.degPos(leftAmplitude * baseAngle1);
        positions[LEFT_FRONT_BACK_ID] = krs.degPos(leftAmplitude * baseAngle2);
        positions[6] = krs.degPos(angle3);  // 左の傾き制御
        
        // 速度の設定
        for (int i = 1; i <= 6; i++) {
            speeds[i] = currentSpeed;
        }
        
        // 位置と速度を設定
        sendVec2ServoPos(positions, speeds);
        
    } catch (...) {
        errorStatus.servoError = true;
        errorStatus.errorMessage = "Servo control error in SwimMode";
    }
}


    // void handleRaiseMode(const SwimParameters& params) {
    //     WingUpMode mode = messageProcessor.getCurrentWingMode();
    //     double angle = params.wingDeg;
    //     if (isAngleValid(angle)) {
    //         switch (mode) {
    //             case WingUpMode::RIGHT:
    //                 krs.setPos(BODY_SERVO_ID, krs.degPos(angle));
    //                 break;
    //             case WingUpMode::LEFT:
    //                 krs.setPos(BODY_SERVO_ID, krs.degPos(-angle));
    //                 break;
    //             case WingUpMode::BOTH:
    //                 krs.setPos(BODY_SERVO_ID, krs.degPos(angle));
    //                 break;
    //         }
    //     }
    // }

    void handleRaiseMode(const SwimParameters& params) {
    // パラメータの妥当性チェック
    // if (params.maxAngleDeg < 0) {
    //     errorStatus.errorMessage = "Invalid parameters in RaiseMode";
    //     return;
    // }

    // モード遷移の処理
    if (previousMode != CrushMode::RAISE) {
        previousMode = CrushMode::RAISE;
    }

    int positions[7] = {0};  // すべて0で初期化
    int speeds[7] = {BASE_SPEED};  // 一定速度で動作
    
    WingUpMode mode = messageProcessor.getCurrentWingMode();
    
    try {
        switch (mode) {
            case WingUpMode::RIGHT:
                // 右側のヒレを上げる
                positions[RIGHT_UP_DOWN_ID] = krs.degPos(0);           // ID:1 は 0度
                positions[RIGHT_FRONT_BACK_ID] = krs.degPos(params.maxAngleDeg);  // ID:2 は最大角度
                positions[3] = krs.degPos(0);           // ID:3 は 0度
                break;
                
            case WingUpMode::LEFT:
                // 左側のヒレを上げる
                positions[LEFT_UP_DOWN_ID] = krs.degPos(0);           // ID:4 は 0度
                positions[LEFT_FRONT_BACK_ID] = krs.degPos(params.maxAngleDeg);   // ID:5 は最大角度
                positions[6] = krs.degPos(0);           // ID:6 は 0度
                break;
                
            case WingUpMode::BOTH:
                // 両方のヒレを上げる
                positions[RIGHT_UP_DOWN_ID] = krs.degPos(0);           // ID:1 は 0度
                positions[RIGHT_FRONT_BACK_ID] = krs.degPos(params.maxAngleDeg);  // ID:2 は最大角度
                positions[3] = krs.degPos(0);           // ID:3 は 0度
                
                positions[LEFT_UP_DOWN_ID] = krs.degPos(0);           // ID:4 は 0度
                positions[LEFT_FRONT_BACK_ID] = krs.degPos(params.maxAngleDeg);   // ID:5 は最大角度
                positions[6] = krs.degPos(0);           // ID:6 は 0度
                break;
        }
        
        // 位置と速度を設定
        sendVec2ServoPos(positions, speeds);
        
    } catch (...) {
        errorStatus.servoError = true;
        errorStatus.errorMessage = "Servo control error in RaiseMode";
    }
}
    void handleEmergencySurface() {
        int positions[SERVO_NUM] = {0};
        int speeds[SERVO_NUM] = {BASE_SPEED};  // 緊急時は一定速度
        
        // すべてのサーボを適切な位置に
        positions[RIGHT_UP_DOWN_ID] = krs.degPos(DEFAULT_SURFACE_ANGLE);    // 上向きに
        positions[RIGHT_FRONT_BACK_ID] = krs.degPos(0);    // 中立位置
        positions[3] = krs.degPos(0);                      // 中立位置
        
        positions[LEFT_UP_DOWN_ID] = krs.degPos(DEFAULT_SURFACE_ANGLE);     // 上向きに
        positions[LEFT_FRONT_BACK_ID] = krs.degPos(0);     // 中立位置
        positions[6] = krs.degPos(0);                      // 中立位置
        
        sendVec2ServoPos(positions, speeds);
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