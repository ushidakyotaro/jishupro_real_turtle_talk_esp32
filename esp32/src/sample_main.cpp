#include <Arduino.h>
#include <IcsHardSerialClass.h>
#include "wifi_connection.h"
#include "message_processor.h"

// サーボ設定用構造体
struct ServoConfig {
    const byte EN_PIN;
    const byte RX_PIN;
    const byte TX_PIN;
    const long BAUDRATE;
    const int TIMEOUT;
    int servoCount;  // サーボの数
};

// エラーステータス構造体
struct ErrorStatus {
    bool wifiDisconnected = false;
    bool servoError = false;
    String errorMessage;
};

class RobotBase {
protected:
    IcsHardSerialClass* krs;
    WiFiConnection wifiConnection;
    MessageProcessor messageProcessor;
    RobotMode currentMode;
    RobotMode previousMode;
    MotionParameters currentParams;
    ErrorStatus errorStatus;
    int servoCount;

    // 通信関連の定数
    const unsigned long WIFI_RECONNECT_INTERVAL = 5000;
    const unsigned long CLIENT_TIMEOUT = 5000;
    const unsigned long MOTION_UPDATE_INTERVAL = 50;

public:
    RobotBase(const ServoConfig& config) : 
        servoCount(config.servoCount),
        currentMode(RobotMode::SERVO_OFF),
        previousMode(RobotMode::SERVO_OFF) {
        
        krs = new IcsHardSerialClass(&Serial2, config.EN_PIN, config.BAUDRATE, config.TIMEOUT);
    }

    virtual ~RobotBase() {
        setServoOff();
        if (krs) delete krs;
    }

    // システム初期化
    static void initializeSystem(const ServoConfig& config) {
        Serial.begin(115200);
        Serial2.begin(config.BAUDRATE, SERIAL_8E1, config.RX_PIN, config.TX_PIN);
        delay(100);

        // EN_PINの設定
        pinMode(config.EN_PIN, OUTPUT);
        digitalWrite(config.EN_PIN, HIGH);
        delay(50);
    }

    // メインループ
    virtual void loop() {
        static unsigned long lastMotionUpdate = 0;
        unsigned long currentTime = millis();

        // WiFi接続管理
        wifiConnection.handleConnection();

        // クライアント通信処理
        if (wifiConnection.isConnected()) {
            WiFiClient client = wifiConnection.getServer()->available();
            if (client) {
                handleClientCommunication(client);
            }
        }

        // モーション更新
        if (currentTime - lastMotionUpdate >= MOTION_UPDATE_INTERVAL) {
            updateMotion();
            lastMotionUpdate = currentTime;
        }
    }

protected:
    // サーボOFF
    void setServoOff() {
        for (int i = 0; i < servoCount; ++i) {
            krs->setFree(i);
        }
    }

    // サーボ位置設定
    void setServoPositions(const int* positions, const int* speeds = nullptr) {
        static int defaultSpeed = 30;
        
        for (int i = 0; i < servoCount; ++i) {
            if (speeds) {
                krs->setSpd(i, speeds[i]);
            } else {
                krs->setSpd(i, defaultSpeed);
            }
            krs->setPos(i, positions[i]);
            delay(1);
        }
    }

    // 初期位置への移動
    void handleInitMode() {
        int* positions = new int[servoCount];
        for (int i = 0; i < servoCount; ++i) {
            positions[i] = krs->degPos(0);  // 全サーボを0度に
        }
        setServoPositions(positions);
        delete[] positions;
    }

    // モーション更新（派生クラスで実装）
    virtual void updateMotion() {
        switch (currentMode) {
            case RobotMode::SERVO_OFF:
                setServoOff();
                break;
            case RobotMode::INIT_POSE:
                handleInitMode();
                break;
            // その他のモーションは派生クラスで実装
            default:
                break;
        }
    }

private:
    void handleClientCommunication(WiFiClient& client) {
        if (messageProcessor.processMessage(client)) {
            currentMode = messageProcessor.getCurrentMode();
            currentParams = messageProcessor.getCurrentParams();
        }
    }
};

// メイン実装例
const ServoConfig DEFAULT_CONFIG = {
    .EN_PIN = 5,
    .RX_PIN = 16,
    .TX_PIN = 17,
    .BAUDRATE = 1250000,
    .TIMEOUT = 20,
    .servoCount = 7  // デフォルトのサーボ数
};

class MyRobot : public RobotBase {
public:
    MyRobot(const ServoConfig& config) : RobotBase(config) {}

protected:
    // 必要に応じて updateMotion をオーバーライド
    void updateMotion() override {
        RobotBase::updateMotion();
        // カスタムモーションの実装
    }
};

MyRobot* robot;

void setup() {
    RobotBase::initializeSystem(DEFAULT_CONFIG);
    robot = new MyRobot(DEFAULT_CONFIG);
}

void loop() {
    robot->loop();
}