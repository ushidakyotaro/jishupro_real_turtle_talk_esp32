#include <Arduino.h>
#include <IcsHardSerialClass.h>
#include "wifi_connection.h"
#include "message_processor.h"
#include "motion_patterns.h"

// サーボ設定
const byte EN_PIN = 5;
const byte RX_PIN = 16;
const byte TX_PIN = 17;
const long BAUDRATE = 1250000;
const int TIMEOUT = 20;

const int SERVO_NUM = 7;

// グローバル変数として定義
IcsHardSerialClass krs(&Serial2, EN_PIN, BAUDRATE, TIMEOUT);
WiFiConnection wifiConnection;
MessageProcessor messageProcessor;

WiFiClient currentClient;

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
    //static IcsHardSerialClass krs;
    // static WiFiConnection wifiConnection;
    //static WiFiClient currentClient;
    // static MessageProcessor messageProcessor;
    
    CrushMode currentMode;
    CrushMode previousMode;
    ErrorStatus errorStatus;
    
    const double MAX_WING_ANGLE = 25.0;
    const double MIN_WING_ANGLE = -25.0;
    const unsigned long WIFI_RECONNECT_INTERVAL = 5000;
    const unsigned long MODE_TRANSITION_DELAY = 1000;
    
    unsigned long lastMotionUpdate = 0;
    const unsigned long MOTION_UPDATE_INTERVAL = 50;  // 20ms間隔で更新

    virtual void updateMotion() = 0;

public:
    CrushMain() : currentMode(CrushMode::INIT_POSE), 
                  previousMode(CrushMode::SERVO_OFF) {}
    
    virtual ~CrushMain() {
        setServoOff();
    }

    static void initializeSystem() {
        // // LED初期化
        // pinMode(LED_BUILTIN, OUTPUT);
        // digitalWrite(LED_BUILTIN, LOW);

        //wifiのシリアル通信
        Serial.begin(115200); //先に設定する必要が有る


        //モータのシリアル通信
        Serial2.begin(BAUDRATE, SERIAL_8E1, RX_PIN, TX_PIN, false, TIMEOUT);
        delay(100);
        krs.begin();

        // EN_PINの設定を追加 この部分を消すと通信はうまく行くがサーボがonにならない
        //この部分があるとサーボがonになるが通信がうまく行かない
        pinMode(EN_PIN, OUTPUT);
        digitalWrite(EN_PIN, HIGH); 
        delay(50); 



        if (wifiConnection.begin()) {
            Serial.println("WiFi Connected");
            Serial.println(WiFi.localIP());
        }
    }

    virtual void loop() {
        static unsigned long lastClientActivity = 0;
        const unsigned long CLIENT_TIMEOUT = 5000;  // 5秒のタイムアウト
        static bool isTimeout = false;

        wifiConnection.handleConnection();

        
        if (wifiConnection.isConnected()) {
            WiFiClient client = wifiConnection.getServer()->available();
            
            if (client) {
                Serial.println("Client connected");
                wifiConnection.setClientConnected(true);
                lastClientActivity = millis();
                isTimeout = false;
                
                // test_tcp_ics.cppと同様の処理構造に修正
                while (client.connected()) {
                    wifiConnection.handleConnection();

                    // タイムアウトチェック
                    if (millis() - lastClientActivity > CLIENT_TIMEOUT) {
                        if (!isTimeout) {
                            Serial.println("Client timeout - switching to SERVO_OFF");
                            currentMode = CrushMode::SERVO_OFF;
                            setServoOff();
                            isTimeout = true;
                        }
                    }       

                    if (messageProcessor.processMessage(client)) {
                        lastClientActivity = millis();  // メッセージを受信したら時間を更新
                        isTimeout = false; 

                        // メッセージを受信したときだけモーション更新
                        auto mode = messageProcessor.getCurrentMode();
                        auto params = messageProcessor.getCurrentParams();
                        
                        // デバッグ出力を追加
                        Serial.printf("Mode: %d, Period: %.2f, Wing: %.1f, Max: %.1f, Y: %.2f\n",
                            static_cast<int>(mode),
                            params.periodSec,
                            params.wingDeg,
                            params.maxAngleDeg,
                            params.yRate);

                        updateMotion();  // 各クラスで実装される処理
                    }

                    // 連続的なモーション更新
                    if (!isTimeout && (currentMode == CrushMode::STAY || currentMode == CrushMode::SWIM)) {
                        unsigned long currentTime = millis();
                        if (currentTime - lastMotionUpdate >= MOTION_UPDATE_INTERVAL) {
                            updateMotion();
                            lastMotionUpdate = currentTime;
                        }
                    }
                }
                
                wifiConnection.setClientConnected(false);
                Serial.println("Client disconnected");
            }
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
        static unsigned long lastReconnectAttempt = 0;
        const unsigned long RECONNECT_INTERVAL = 5000;  // 5秒ごとに再接続を試みる
        
        unsigned long currentTime = millis();
        if (currentTime - lastReconnectAttempt >= RECONNECT_INTERVAL) {
            if (wifiConnection.begin()) {
                Serial.println("WiFi Reconnected");
            }
            lastReconnectAttempt = currentTime;
        }
    }
    
};

// 静的メンバの定義
//IcsHardSerialClass CrushMain::krs(&Serial2, EN_PIN, BAUDRATE, TIMEOUT);

//WiFiConnection CrushMain::wifiConnection;
//MessageProcessor CrushMain::messageProcessor;

class CrushBody : public CrushMain {
private:
    SplineInterpolator splineInterpolator;
    std::vector<WingMotionPoint> currentPattern;
    CycleTimer cycleTimer;
    //const int BODY_SERVO_ID = 0;
    unsigned long lastUpdateTime = 0;
    
    
    CrushMode currentMode;

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

    unsigned long lastMotionUpdate = 0;
    const unsigned long MOTION_UPDATE_INTERVAL = 20;  // 20ms間隔で更新


protected:
    void updateMotion() override {
        auto mode = messageProcessor.getCurrentMode();
        auto params = messageProcessor.getCurrentParams();
        // WiFi通信が来ても、モードが同じなら継続
        if (mode != currentMode) {
            // モードが変更された時のみ初期化処理を行う
            Serial.printf("Mode changed from %d to %d\n", static_cast<int>(currentMode), static_cast<int>(mode));
            currentMode = mode;
            cycleTimer.reset();  // タイマーをリセット
            // その他の初期化処理
        }
        
        switch (currentMode) {
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
    // void sendVec2ServoPos(int posVec[SERVO_NUM], int speedVec[SERVO_NUM]){
    //     static int defaultSpeed[SERVO_NUM] = {127, 127, 127, 127, 127, 127, 127};

    //     if (speedVec == nullptr) {
    //         speedVec = defaultSpeed;
    //     }
    //     for (int i = 0; i < SERVO_NUM; ++i) {
    //         //if (isAngleValid(posVec[i])) {
    //             while (krs.setSpd(i,speedVec[i]) == -1) {
    //                 delay(1);
    //             }
    //             while (krs.setPos(i, posVec[i]) == -1) {
    //                 delay(1);
    //             }
    //         //}
    //         Serial.printf("Servo %d: pos=%d, speed=%d\n", i, posVec[i], speedVec[i]);
    //     }
        
    // }

    void sendVec2ServoPos(int posVec[SERVO_NUM], int speedVec[SERVO_NUM]){
        //static int defaultSpeed[SERVO_NUM] = {127, 127, 127, 127, 127, 127, 127};
        static int defaultSpeed[SERVO_NUM] = {50, 50, 50, 50, 50, 50, 50};
        if (speedVec == nullptr) {
            speedVec = defaultSpeed;
        }
        
        // 各サーボについて最大5回までリトライ
        const int MAX_RETRY = 5;
        
        for (int i = 1; i < SERVO_NUM; ++i) {
        //for (int i = 0; i < SERVO_NUM; ++i) {
            // スピード設定
            int retryCount = 0;
            bool speedSet = false;
            while (retryCount < MAX_RETRY && !speedSet) {
                if (krs.setSpd(i, speedVec[i]) != -1) {
                    speedSet = true;
                } else {
                    retryCount++;
                    if (retryCount == MAX_RETRY) {
                        Serial.printf("Failed to set speed for servo %d\n", i);
                    }
                }
            }
            
            // ポジション設定
            retryCount = 0;
            bool posSet = false;
            while (retryCount < MAX_RETRY && !posSet) {
                if (krs.setPos(i, posVec[i]) != -1) {
                    posSet = true;
                } else {
                    retryCount++;
                    if (retryCount == MAX_RETRY) {
                        Serial.printf("Failed to set position for servo %d\n", i);
                    }
                }
            }
            
            Serial.printf("Servo %d: pos=%d, speed=%d\n", i, posVec[i], speedVec[i]);
            
            // 各サーボ設定後に短い待機時間を入れる
            delay(1);
        }
    }

    // void handleInitMode() {
    //     // if (currentMode == CrushMode::INIT_POSE) {
    //         int pos = krs.degPos(0);
    //         for (int i = 1; i < SERVO_NUM; ++i) {
    //             while (krs.setPos(i, pos) == -1) {
    //                 delay(1);
    //             }
    //         }
    //     }
        void handleInitMode() {
        // if (currentMode == CrushMode::INIT_POSE) {
            int positions[SERVO_NUM];
            int pos = krs.degPos(0);
            for (int i = 1; i < SERVO_NUM; ++i) {
                positions[i] = pos;
            }
            sendVec2ServoPos(positions, nullptr);
        }

    // void handleStayMode(const SwimParameters& params) {
    //     // シンプルな上下運動のみを実装///////////////////////////
    //     static bool isUp = false;
    //     const int UP_ANGLE = 30;    // 上げる角度
    //     const int DOWN_ANGLE = -30;  // 下げる角度
        
    //     // 1秒ごとに切り替え
    //     if (millis() - cycleTimer.startTime > 1000) {
    //         isUp = !isUp;  // 状態を反転
    //         cycleTimer.reset();
            
    //         int targetAngle = isUp ? UP_ANGLE : DOWN_ANGLE;
            
    //         // すべてのサーボに同じ角度を設定
    //         int positions[SERVO_NUM] = {0};  // 0番は使わない
    //         for(int i = 1; i < SERVO_NUM; i++) {
    //             positions[i] = krs.degPos(targetAngle);
    //         }
            
    //         int speeds[SERVO_NUM] = {127, 127, 127, 127, 127, 127, 127};
    //         sendVec2ServoPos(positions, speeds);
            
    //         // デバッグ出力
    //         Serial.printf("Stay Mode: Moving to angle %d\n", targetAngle);
    //     }

    // }

    void handleStayMode(const SwimParameters& params) {
        // 固定値の設定
        const double PERIOD_MS = 3000.0;  // 3秒周期
        const double MAX_ANGLE = 20.0;    // 振幅±20度
        
        // 現在の時間から角度を計算
        unsigned long currentTime = millis();
        double timeRatio = fmod(currentTime, PERIOD_MS) / PERIOD_MS;  // 0.0 ~ 1.0の値
        double currentAngle = MAX_ANGLE * sin(TWO_PI * timeRatio);    // -30 ~ +30度
        
        // サーボの位置と速度を設定
        int positions[SERVO_NUM] = {0};  // 0番は使わない
        int speeds[SERVO_NUM] = {127, 127, 127, 127, 127, 127, 127};  // 一定速度
        
        // // 全サーボ（1-6番）に同じ角度を設定
        // for(int i = 1; i < SERVO_NUM; i++) {
        //     positions[i] = krs.degPos(currentAngle);
        // }
        // positions[i] = krs.degPos(currentAngle);

            // wingdegをラジアンに変換
            double WING_DEG = 0.0;

            double wingRad = WING_DEG * PI / 180.0;
            
            // サーボの角度を計算
            double angle1 = currentAngle * cos(wingRad);  // サーボ1,4用
            double angle2 = currentAngle * sin(wingRad);  // サーボ2,5用
        
            // 各サーボに角度を設定
        positions[1] = krs.degPos(angle1);  // 右の上下
        positions[2] = krs.degPos(angle2);  // 右の前後
        positions[4] = krs.degPos(-angle1);  // 左の上下
        positions[5] = krs.degPos(-angle2);  // 左の前後
        
        // 3番と6番は0度に維持
        positions[3] = krs.degPos(0);
        positions[6] = krs.degPos(0);
        
        // サーボに送信
        sendVec2ServoPos(positions, speeds);
        
        // デバッグ出力（500msごと）
        static unsigned long lastDebugTime = 0;
        if (currentTime - lastDebugTime > 500) {
            Serial.printf("Stay Mode: Current angle = %.2f\n", currentAngle);
            lastDebugTime = currentTime;
        }
    }

// void handleStayMode(const SwimParameters& params) {
//     // モード遷移の処理（これは保持）
//     if (previousMode != CrushMode::STAY) {
//         cycleTimer.reset();
//         previousMode = CrushMode::STAY;
//     }

//     // 周期の更新確認
//     if (cycleTimer.periodSec != params.periodSec) {
//         cycleTimer.start(params.periodSec);
//     }

//     // 時間比率の計算（0.0 ~ 1.0）
//     double timeRatio = cycleTimer.getCurrentTimeRatio();
    
//     // wingdegをラジアンに変換
//     double wingRad = params.wingDeg * PI / 180.0;
    
//     // サーボの角度を計算
//     double angle1 = params.maxAngleDeg * sin(wingRad) * sin(TWO_PI * timeRatio);  // サーボ1,4用
//     double angle2 = params.maxAngleDeg * cos(wingRad) * sin(TWO_PI * timeRatio);  // サーボ2,5用
    
//     // サーボの位置と速度を設定
//     int positions[SERVO_NUM] = {0};  // 0番は使わない
//     int speeds[SERVO_NUM] = {127, 127, 127, 127, 127, 127, 127};
    
//     // 各サーボに角度を設定
//     positions[1] = krs.degPos(angle1);  // 右の上下
//     positions[2] = krs.degPos(angle2);  // 右の前後
//     positions[4] = krs.degPos(angle1);  // 左の上下
//     positions[5] = krs.degPos(angle2);  // 左の前後
    
//     // 3番と6番は0度に維持
//     positions[3] = krs.degPos(0);
//     positions[6] = krs.degPos(0);
    
//     // サーボに送信
//     sendVec2ServoPos(positions, speeds);

//     // デバッグ出力（500msごと）
//     static unsigned long lastDebugTime = 0;
//     if (millis() - lastDebugTime > 500) {
//         Serial.printf("Stay Mode: angle1=%.2f, angle2=%.2f\n", angle1, angle2);
//         lastDebugTime = millis();
//     }
// }
    

void handleSwimMode(const SwimParameters& params) {
    // 固定値の設定
    const double PERIOD_MS = 2000.0;      // 2秒周期
    const double MAX_ANGLE = 20.0;        // 振幅±30度
    const double WING_DEG = 20.0;         // 翼角度
    const double RIGHT_RATE = 1.0; //1.2       // 右の振幅率（1.0より大きいと右に曲がる）
    const double LEFT_RATE = 1.0;  //0.8       // 左の振幅率
    const double WING_ROTATION = 30.0;    // 翼の回転角度
    
    // 現在の時間から基本角度を計算
    unsigned long currentTime = millis();
    double timeRatio = fmod(currentTime, PERIOD_MS) / PERIOD_MS;
    double baseAngle = MAX_ANGLE * sin(TWO_PI * timeRatio);
    
    // 翼の方向計算
    double wingRad = WING_DEG * PI / 180.0;
    
    // 左右の振幅調整
    double rightAngle1 = baseAngle * RIGHT_RATE * cos(wingRad);
    double rightAngle2 = baseAngle * RIGHT_RATE * sin(wingRad);
    double leftAngle1 = baseAngle * LEFT_RATE * cos(wingRad);
    double leftAngle2 = baseAngle * LEFT_RATE * sin(wingRad);
    
    // 3番と6番サーボの制御（前進動作用）
    double rotationAngle = 0.0;
    if (cos(TWO_PI * timeRatio) < 0) {  // 後ろに動かすとき
        rotationAngle = WING_ROTATION;
    }
    
    int positions[SERVO_NUM] = {0};
    int speeds[SERVO_NUM] = {127, 127, 127, 127, 127, 127, 127};
    
    // 各サーボに角度を設定
    positions[1] = krs.degPos(rightAngle1);   // 右の上下
    positions[2] = krs.degPos(rightAngle2);   // 右の前後
    positions[3] = krs.degPos(rotationAngle); // 右の回転
    positions[4] = krs.degPos(leftAngle1);   // 左の上下
    positions[5] = krs.degPos(leftAngle2);   // 左の前後
    positions[6] = krs.degPos(rotationAngle); // 左の回転
    
    sendVec2ServoPos(positions, speeds);
    
    // デバッグ出力
    static unsigned long lastDebugTime = 0;
    if (currentTime - lastDebugTime > 500) {
        Serial.printf("Swim Mode: Base=%.2f, Right=%.2f, Left=%.2f, Rotation=%.2f\n",
            baseAngle, rightAngle1, leftAngle1, rotationAngle);
        lastDebugTime = currentTime;
    }
}

//     void handleSwimMode(const SwimParameters& params) {
//         // パラメータの妥当性チェック


//         // モード遷移の処理
//         if (previousMode != CrushMode::SWIM) {
//             cycleTimer.reset();
//             previousMode = CrushMode::SWIM;
//         }

//         // 周期の更新確認
//         if (cycleTimer.periodSec != params.periodSec) {
//             cycleTimer.start(params.periodSec);
//         }

//         double timeRatio = cycleTimer.getCurrentTimeRatio();
//         double wingRad = params.wingDeg * PI / 180.0;
        
//         // 左右の振幅調整（yRateに基づく）
//         double rightAmplitude = params.maxAngleDeg * (1.0 + params.yRate) / 2.0;
//         double leftAmplitude = params.maxAngleDeg * (1.0 - params.yRate) / 2.0;
        
//         // 基本の角度計算
//         double baseAngle1 = sin(wingRad) * sin(TWO_PI * timeRatio);  // 上下運動
//         double baseAngle2 = cos(wingRad) * sin(TWO_PI * timeRatio);  // 前後運動
        
//         // 3番と6番サーボの角度計算（前進/後退用）
//         double angle3 = 0.0;
//         if (!params.isBackward) {  // 前進の場合
//             // baseAngle2が正から負に変化する領域（前から後ろに動かすとき）で90度
//             if (cos(TWO_PI * timeRatio) < 0) {
//                 angle3 = WING_ROTATION_ANGLE;
//             }
//         } else {  // 後退の場合
//             // baseAngle2が負から正に変化する領域（後ろから前に動かすとき）で90度
//             if (cos(TWO_PI * timeRatio) > 0) {
//                 angle3 = WING_ROTATION_ANGLE;
//             }
//         }

//         int positions[SERVO_NUM] = {0};
//         int speeds[SERVO_NUM] = {0};
        
//         // 速度計算
//         double speedFactor = abs(cos(TWO_PI * timeRatio));
//         int currentSpeed = BASE_SPEED + (int)(SPEED_VARIATION * speedFactor);
        
//         try {
//             // 右腕の設定
//             positions[RIGHT_UP_DOWN_ID] = krs.degPos(rightAmplitude * baseAngle1);
//             positions[RIGHT_FRONT_BACK_ID] = krs.degPos(rightAmplitude * baseAngle2);
//             positions[3] = krs.degPos(angle3);  // 右の傾き制御
            
//             // 左腕の設定
//             positions[LEFT_UP_DOWN_ID] = krs.degPos(leftAmplitude * baseAngle1);
//             positions[LEFT_FRONT_BACK_ID] = krs.degPos(leftAmplitude * baseAngle2);
//             positions[6] = krs.degPos(angle3);  // 左の傾き制御
            
//             // 速度の設定
//             for (int i = 1; i <= 6; i++) {
//                 speeds[i] = currentSpeed;
//             }
            
//             // 位置と速度を設定
//             sendVec2ServoPos(positions, speeds);
            
//         } catch (...) {
//             errorStatus.servoError = true;
//             errorStatus.errorMessage = "Servo control error in SwimMode";
//         }
// }



void handleRaiseMode(const SwimParameters& params) {
    unsigned long currentTime = millis();
    int positions[SERVO_NUM] = {0};  // すべて0で初期化
    int speeds[SERVO_NUM] = {127, 127, 40, 127, 127, 40, 127};  // servo2,5のみ速度80
    
    // servo1,3,4,6は0度
    positions[1] = krs.degPos(0);
    positions[3] = krs.degPos(0);
    positions[4] = krs.degPos(0);
    positions[6] = krs.degPos(0);
    
    // servo2,5は30度
    positions[2] = krs.degPos(20);
    positions[5] = krs.degPos(-20);
    
    sendVec2ServoPos(positions, speeds);
    
    // デバッグ出力
    static unsigned long lastDebugTime = 0;
    if (millis() - lastDebugTime > 500) {
        Serial.printf("Raise Mode: servo2,5 at 30 degrees, speed 80\n");
        lastDebugTime = currentTime;
    }
}

//     void handleRaiseMode(const SwimParameters& params) {
//     // パラメータの妥当性チェック
//     // if (params.maxAngleDeg < 0) {
//     //     errorStatus.errorMessage = "Invalid parameters in RaiseMode";
//     //     return;
//     // }

//     // モード遷移の処理
//     if (previousMode != CrushMode::RAISE) {
//         previousMode = CrushMode::RAISE;
//     }

//     int positions[7] = {0};  // すべて0で初期化
//     int speeds[7] = {BASE_SPEED};  // 一定速度で動作
    
//     WingUpMode mode = messageProcessor.getCurrentWingMode();
    
//     try {
//         switch (mode) {
//             case WingUpMode::RIGHT:
//                 // 右側のヒレを上げる
//                 positions[RIGHT_UP_DOWN_ID] = krs.degPos(0);           // ID:1 は 0度
//                 positions[RIGHT_FRONT_BACK_ID] = krs.degPos(params.maxAngleDeg);  // ID:2 は最大角度
//                 positions[3] = krs.degPos(0);           // ID:3 は 0度
//                 break;
                
//             case WingUpMode::LEFT:
//                 // 左側のヒレを上げる
//                 positions[LEFT_UP_DOWN_ID] = krs.degPos(0);           // ID:4 は 0度
//                 positions[LEFT_FRONT_BACK_ID] = krs.degPos(params.maxAngleDeg);   // ID:5 は最大角度
//                 positions[6] = krs.degPos(0);           // ID:6 は 0度
//                 break;
                
//             case WingUpMode::BOTH:
//                 // 両方のヒレを上げる
//                 positions[RIGHT_UP_DOWN_ID] = krs.degPos(0);           // ID:1 は 0度
//                 positions[RIGHT_FRONT_BACK_ID] = krs.degPos(params.maxAngleDeg);  // ID:2 は最大角度
//                 positions[3] = krs.degPos(0);           // ID:3 は 0度
                
//                 positions[LEFT_UP_DOWN_ID] = krs.degPos(0);           // ID:4 は 0度
//                 positions[LEFT_FRONT_BACK_ID] = krs.degPos(params.maxAngleDeg);   // ID:5 は最大角度
//                 positions[6] = krs.degPos(0);           // ID:6 は 0度
//                 break;
//         }
        
//         // 位置と速度を設定
//         sendVec2ServoPos(positions, speeds);
        
//     } catch (...) {
//         errorStatus.servoError = true;
//         errorStatus.errorMessage = "Servo control error in RaiseMode";
//     }
// }
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

// class CrushMouth : public CrushMain {
// private:
//     const int MOUTH_SERVO_ID = 0;
//     const double MOUTH_OPEN_ANGLE = 30.0;
//     const double MOUTH_CLOSE_ANGLE = 0.0;
//     unsigned long lastMouthUpdate = 0;
//     const unsigned long MOUTH_UPDATE_INTERVAL = 100;

// protected:
//     void updateMotion() override {
//         unsigned long currentTime = millis();
//         if (currentTime - lastMouthUpdate >= MOUTH_UPDATE_INTERVAL) {
//             bool shouldOpen = messageProcessor.getMouthOpen();
//             double targetAngle = shouldOpen ? MOUTH_OPEN_ANGLE : MOUTH_CLOSE_ANGLE;
//             krs.setPos(MOUTH_SERVO_ID, krs.degPos(targetAngle));
//             lastMouthUpdate = currentTime;
//         }
//     }
// };

// グローバルインスタンス
CrushBody body;
//CrushMouth mouth;
//WiFiClient CrushMain::currentClient;

void setup() {
    CrushMain::initializeSystem();
}

void loop() {
    body.loop();
    //mouth.loop(); //とりあえずコメントアウト
}