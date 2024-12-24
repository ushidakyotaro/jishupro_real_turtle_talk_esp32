//src/message_processor.cpp
#include "message_processor.h"

MessageProcessor::MessageProcessor() 
    : currentMode(CrushMode::INIT_POSE)
    , currentWingMode(WingUpMode::BOTH)
    , isMouthOpen(false) {
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
            if (subCommand <= 5) {
                currentMode = static_cast<CrushMode>(subCommand);
                sendResponse(client, 0x00);
            } else {
                sendResponse(client, 0xE1);
            }
            break;

        case 0x02: // 遊泳パラメータ設定
            if (client.available() >= 9) {
                uint8_t buffer[9];
                size_t bytesRead = client.readBytes(buffer, 9);
                if (bytesRead != 9) {
                    sendResponse(client, 0xE3);
                    break;
                }

                float period = bytesToFloat(buffer);
                float wingDeg = bytesToInt16(buffer + 4) / 10.0f;
                float maxAngle = bytesToInt16(buffer + 6) / 10.0f;
                float yRate = static_cast<int8_t>(buffer[8]) / 100.0f;

                if (wingDeg >= -45.0 && wingDeg <= 45.0 &&
                    maxAngle >= -45.0 && maxAngle <= 45.0 &&
                    yRate >= -1.0 && yRate <= 1.0 &&
                    period > 0) {
                    
                    currentParams.periodSec = period;
                    currentParams.wingDeg = wingDeg;
                    currentParams.maxAngleDeg = maxAngle;
                    currentParams.yRate = yRate;
                    sendResponse(client, 0x00);
                } else {
                    sendResponse(client, 0xE2);
                }
            } else {
                sendResponse(client, 0xE3);
            }
            break;

        case 0x03: // 翼制御
            if (client.available() >= 2) {
                uint8_t angleBytes[2];
                size_t bytesRead = client.readBytes(angleBytes, 2);
                if (bytesRead != 2) {
                    sendResponse(client, 0xE3);
                    break;
                }

                float angle = bytesToInt16(angleBytes) / 10.0f;
                if (angle >= -45.0 && angle <= 45.0 && subCommand >= 1 && subCommand <= 3) {
                    currentWingMode = static_cast<WingUpMode>(subCommand);
                    currentParams.wingDeg = angle;
                    sendResponse(client, 0x00);
                } else {
                    sendResponse(client, 0xE2);
                }
            } else {
                sendResponse(client, 0xE3);
            }
            break;

        case 0x04: // 口制御
            isMouthOpen = (subCommand & 0x01) != 0;
            sendResponse(client, 0x00);
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

void MessageProcessor::statusResponse(WiFiClient& client) {
    uint8_t response[8] = {0};
    response[0] = static_cast<uint8_t>(currentMode);
    int16_t currentAngle = static_cast<int16_t>(currentParams.wingDeg * 10);
    memcpy(response + 1, &currentAngle, 2);
    client.write(response, sizeof(response));
}