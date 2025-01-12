# ESP32 Firmware Development

This directory contains the ESP32 firmware for the Crush robot project.

## Directory Structure
```plaintext
esp32/
├── include/             # Shared header files
│   ├── credentials.h    # WiFi credentials
│   ├── message_processor.h # Message processing
│   ├── motion_patterns.h  # Motion definitions
│   └── wifi_connection.h  # WiFi connection handling
├── lib/                 # Libraries
│   └── IcsClass/       # Servo motor control
├── src/                # Main source code
│   ├── main.cpp        # Main program
│   ├── message_processor.cpp  # Message processing implementation
│   └── wifi_connection.cpp    # WiFi connection implementation
└── platformio.ini      # PlatformIO configuration
```

## Setup Instructions

### 1. PlatformIO Project Setup
1. Open VSCode
2. Install PlatformIO Extension
3. Open this directory in VSCode:
   ```bash
   code .
   ```
4. Wait for PlatformIO to initialize the project


### 2. WiFi Configuration
1. Copy `include/credentials_template.h` to `include/credentials.h`
2. Edit `credentials.h` with your WiFi settings:
   - Home WiFi credentials
   - UTokyo WiFi credentials (if applicable)
   ```cpp
    // Home WiFi settings
    #define HOME_SSID "your_home_ssid"
    #define HOME_PASSWORD "your_home_password"

    // UTokyo WiFi settings
    #define UTOKYO_SSID "0000UTokyo"
    #define UTOKYO_USERNAME "your_username@wifi.u-tokyo.ac.jp"
    #define UTOKYO_PASSWORD "your_password"
   ```

### 3. Hardware Connection
1. Connect ESP32 to your computer via USB
2. Connect servo motors to their respective pins
3. Connect ICS board to ESP32:
   - EN Pin: GPIO5
   - RX Pin: GPIO16
   - TX Pin: GPIO17
4. Ensure proper power supply for servo motors

### 4. Building and Uploading
1. Open the project in VSCode with PlatformIO
2. Select the appropriate environment:
   - For main program: `esp32dev_main`
   - For servo motor testing: `esp32dev_test_servo`
   - For WiFi testing: `esp32dev_test_wifi`
3. Click Upload button or use PlatformIO CLI:
   ```bash
   # For main program
   pio run -e esp32dev_main -t upload
   
   # For servo testing
   pio run -e esp32dev_test_servo -t upload
   
   # For WiFi testing
   pio run -e esp32dev_test_wifi -t upload

## Testing
### 1. Servo Motor Testing
1. Upload the servo test program:
   ```bash
   pio run -e esp32dev_test_servo -t upload
    ```
2. Monitor the serial output in PlatformIO
3. Check if the servo motors move according to the test sequence

### 2. WiFi Connection Testing

1. Upload the WiFi test program:
```bash
pio run -e esp32dev_test_wifi -t upload
```
2. Monitor serial output in PlatformIO
3. Check the assigned IP address
4. Run the Python client script:
```bash
python ../client/test/client_test_wifi_TCP.py
```

5. LED indicators will show the connection status:
    - Fast Blink (100ms): Client Connected
    - Medium Blink (2500ms/500ms): WiFi Connected, No Client
    - Slow Blink (3000ms): No WiFi Connection


## Usage
- LED Status Indicators:
  - Fast Blink (100ms): Client Connected
  - Medium Blink (2000ms/500ms): WiFi Connected, No Client
  - Slow Blink (3000ms): No WiFi Connection

## Troubleshooting
- If WiFi connection fails, check credentials in `credentials.h`
- For UTokyo WiFi issues, verify your account settings
- Check LED status for connection state
- Monitor serial output for detailed error messages
