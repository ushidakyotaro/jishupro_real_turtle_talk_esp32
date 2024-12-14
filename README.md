# Sea Turtle Character (Crush) Robot and Interactive Turtle Talk Simulation

This project aims to replicate the sea turtle character and turtle talk experience from Tokyo DisneySea, developed as an independent project (2024) at the Department of Mechano-Informatics, The University of Tokyo.

## Project Overview
- Recreation of the Crush character from Finding Nemo/Tokyo DisneySea
- Interactive communication system using ESP32
- Servo motor control for realistic movement
- TCP/IP communication for real-time control

## Hardware Requirements
### Main Components
- ESP32 Development Board
- ICS board produced by Kondo Science
- Kondo Science Servo Motors (KRS Series)
- Power Supply for Servo Motors
- USB Cable for Programming

### Servo Motor Configuration
| ID | Location | Movement | Angle Limits |
|----|----------|----------|--------------|
| 0  | Mouth    | Open/Close | TBD |
| 1  | Right Pitch (Base) | Up/Down | TBD |
| 2  | Right Roll | Front/Back | TBD |
| 3  | Right Yaw | Yaw | TBD |
| 4  | Left Pitch | Up/Down | TBD |
| 5  | Left Roll | Front/Back | TBD |
| 6  | Left Yaw | Yaw | TBD |

#### Nice to Have (Future Implementation)
| ID | Location | Movement | Angle Limits |
|----|----------|----------|--------------|
| 7  | Right Back Fin | Up/Down | TBD |
| 8  | Left Back Fin | Up/Down | TBD |

## Software Requirements
- VSCode with PlatformIO Extension
- Python 3.x for Client Application
- Required PlatformIO Libraries (listed in platformio.ini)

## Project Structure
```plaintext
<project>/
├── include/             # Shared header files
│   ├── credentials.h
│   └── credentials_template.h
├── src/                # Main source code
│   └── main.cpp
├── test/               # Test code
│   ├── test_ics.cpp    # Servo motor test
│   └── test_wifi_TCP.cpp # WiFi/TCP test
└── platformio.ini
```

## Setup Instructions
### 1. Development Environment Setup
1. Install VSCode
2. Install PlatformIO Extension in VSCode
3. Clone this repository:
   ```bash
   git clone https://github.com/ushidakyotaro/jishupro_real_turtle_talk_esp32.git
   ```

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
2. Connect servo motors to their respective pins (pin configuration details to be added)
3. Ensure proper power supply for servo motors

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
python client_test_wifi_TCP.py
```

5. LED indicators will show the connection status:
    - Fast Blink (100ms): Client Connected
    - Medium Blink (2500ms/500ms): WiFi Connected, No Client
    - Slow Blink (3000ms): No WiFi Connection


## Usage
- LED Status Indicators:
  - Fast Blink (100ms): Client Connected
  - Medium Blink (2500ms/500ms): WiFi Connected, No Client
  - Slow Blink (3000ms): No WiFi Connection

## Troubleshooting
- If WiFi connection fails, check credentials in `credentials.h`
- For UTokyo WiFi issues, verify your account settings
- Check LED status for connection state
- Monitor serial output for detailed error messages

## Contributing
This is an independent project for educational purposes. Feel free to fork and improve.

## License
This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## Contact
For questions about this project, please contact:
- Please create an issue on our [GitHub repository](https://github.com/ushidakyotaro/jishupro_real_turtle_talk_esp32/issues)
- All bug reports, feature requests, and general questions are welcome
