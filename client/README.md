# Client Application

This directory contains the PC-side client application for the Crush robot project.

## Overview
The client application provides:
- TCP/IP communication with ESP32
- Control interface for the robot
- Testing utilities for WiFi connection

## Directory Structure
```plaintext
client/
├── src/
│   └── UI/
│       └── client_main.py     # Main GUI application
├── test/
│   └── client_test_wifi_TCP.py  # WiFi connection test client
├── requirements.txt             # Python package requirements
└── README.md                   # This file
```

## Requirements
- Python 3.x
- Network connection (same network as ESP32)

## Setup Instructions

### 1. Python Environment Setup
1. Ensure Python 3.x is installed:
   ```bash
   python --version
   ```

2. Install required packages:
   ```bash
   python -m pip install -r requirements.txt
   ```

### 2. WiFi Connection Testing

1. First, ensure ESP32 is running the WiFi test program
   - Upload the test program to ESP32 using PlatformIO
   - Check the ESP32's IP address from serial monitor

2. Run the test client:
   ```bash
   cd src
   python client_test_wifi_TCP.py
   ```

3. When prompted, enter:
   - ESP32's IP address
   - Test message to send

4. Check the connection status:
   - ESP32's LED indicators will show the connection state
   - Messages will be echoed back from ESP32

### 3. Runnning the Aplication
1. Run the main client application:
```bash
cd src/UI
python client_main.py
```
2. Select your location when prompted:
- Home (IP: 10.1.100.158)
- School (IP: 10.100.82.80)

## User Interface Guide
### Basic Controls

- Connection Management

   - Manual connect/disconnect buttons
   - Connection status display
   - Auto-reconnect on connection loss



### Robot Control Modes

- Main Modes

   - サーボOFF (Servo Off)
   - 初期位置 (Initial Position)
   - 待機 (Stay)
   - 泳ぐ (Swim)
   - 手を上げる (Raise Flippers)
   - 緊急停止 (Emergency Stop)

### Swimming Mode Controls

- Movement Parameters

- Period: 0.5-3.0 seconds
- Wing Angle: 0-90 degrees
- Max Angle: 0-45 degrees
- Balance: -1.0 to 1.0
- Backward Mode toggle


- Direction Controls

   - ↑: Forward
   - ↓: Backward
   - ←: Turn left
   - →: Turn right
   - ・: Stay


### Flipper Controls

   - 左ヒレ↑: Raise left flipper
   - 両ヒレ↑: Raise both flippers
   - 右ヒレ↑: Raise right flipper



### Status Display

- Current mode
- Current angle
- WiFi connection status
- Angle range status
- Current parameter values


## Troubleshooting
- Ensure both PC and ESP32 are on the same network
- Check if the correct IP address is being used
- Verify that port 8000 is not blocked by firewall
- Monitor ESP32's serial output for connection status

## Future Development
- UI implementation for robot control
- Real-time visualization
- Motion sequence programming interface

## Contributing
Feel free to contribute to the client application development:
1. Fork the repository
2. Create your feature branch
3. Commit your changes
4. Submit a pull request
