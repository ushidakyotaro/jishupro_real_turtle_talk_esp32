// message_processor.cpp
#include "message_processor.h"

MessageProcessor::MessageProcessor() 
    : currentMode(RobotMode::INIT_POSE)
{
    // デフォルトパラメータの初期化
    currentParams.param1 = 0.0f;  // 周期
    currentParams.param2 = 0.0f;  // 基本角度
    currentParams.param3 = 0.0f;  // 最大角度
    currentParams.param4 = 0.0f;  // バランス調整
    currentParams.flag1 = false;  // 方向フラグ
}

float MessageProcessor::bytesToFloat(const uint8_t* bytes) {
    float value;
    memcpy(&value, bytes, 4);
    return value;
}

int16_t MessageProcessor::bytesToInt16(const uint8_t* bytes) {
    return (bytes[1] << 8) | bytes[0];
}

void MessageProcessor::sendResponse(WiFiClient& client, uint8_t response) {
    client.write(&response, 1);
}

bool MessageProcessor::processMessage(WiFiClient& client) {
    if (!client.available()) return true;

    uint8_t commandByte = client.read();
    uint8_t commandType = (commandByte >> 4) & 0x0F;
    uint8_t subCommand = commandByte & 0x0F;

    switch (commandType) {
        case 0x01: // モード設定
            if (subCommand <= static_cast<uint8_t>(RobotMode::EMERGENCY)) {
                currentMode = static_cast<RobotMode>(subCommand);
                sendResponse(client, 0x00);
            } else {
                sendResponse(client, 0xE1);
            }
            break;

        case 0x02: // パラメータ設定
            if (client.available() >= 10) {
                uint8_t buffer[10];
                size_t bytesRead = client.readBytes(buffer, 10);
                if (bytesRead != 10) {
                    sendResponse(client, 0xE3);
                    break;
                }

                // パラメータの解析と設定
                currentParams.param1 = bytesToFloat(buffer);  // 周期
                currentParams.param2 = bytesToInt16(buffer + 4) / 10.0f;  // 角度1
                currentParams.param3 = bytesToInt16(buffer + 6) / 10.0f;  // 角度2
                currentParams.param4 = static_cast<int8_t>(buffer[8]) / 100.0f;  // 比率
                currentParams.flag1 = buffer[9] != 0;  // フラグ

                if (validateParameters(currentParams)) {
                    sendResponse(client, 0x00);
                } else {
                    sendResponse(client, 0xE2);
                }
            }
            break;

        case 0x0F: // ステータス要求
            statusResponse(client);
            break;

        default:
            sendResponse(client, 0xE0);
            break;
    }
    return true;
}

bool MessageProcessor::validateParameters(const MotionParameters& params) {
    // パラメータの妥当性チェック
    return params.param1 > 0 && 
           params.param2 >= -45.0f && params.param2 <= 45.0f &&
           params.param3 >= -45.0f && params.param3 <= 45.0f &&
           params.param4 >= -1.0f && params.param4 <= 1.0f;
}

void MessageProcessor::statusResponse(WiFiClient& client) {
    uint8_t response[8] = {0};
    response[0] = static_cast<uint8_t>(currentMode);
    int16_t currentAngle = static_cast<int16_t>(currentParams.param2 * 10);
    response[3] = 0;  // エラーフラグ
    client.write(response, sizeof(response));
}