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
- Kondo Science Servo Motors (KRS Series)
- Power Supply for Servo Motors
- USB Cable for Programming

### Servo Motor Configuration
| ID | Location | Movement | Angle Limits |
|----|----------|----------|--------------|
| 0  | Mouth    | Open/Close | TBD |
| 1  | Right Pitch (Base) | Front/Back | TBD |
| 2  | Right Roll | Roll | TBD |
| 3  | Right Yaw | Left/Right | TBD |
| 4  | Left Pitch | Front/Back | TBD |
| 5  | Left Roll | Roll | TBD |
| 6  | Left Yaw | Left/Right | TBD |

#### Nice to Have (Future Implementation)
| ID | Location | Movement | Angle Limits |
|----|----------|----------|--------------|
| 7  | Right Back Fin | Up/Down | TBD |
| 8  | Left Back Fin | Up/Down | TBD |

## Software Requirements
- VSCode with PlatformIO Extension
- Python 3.x for Client Application
- Required PlatformIO Libraries (listed in platformio.ini)

## Setup Instructions
### 1. Development Environment Setup
1. Install VSCode
2. Install PlatformIO Extension in VSCode
3. Clone this repository:
   ```bash
   git clone https://github.com/your-username/jishupro_real_turtle_talk_esp32.git
   ```

### 2. WiFi Configuration
1. Copy `src/credentials_template.h` to `src/credentials.h`
2. Edit `credentials.h` with your WiFi settings:
   - Home WiFi credentials
   - UTokyo WiFi credentials (if applicable)
   ```cpp
   #define HOME_SSID "your_home_ssid"
   #define HOME_PASSWORD "your_home_password"
   #define UTOKYO_USERNAME "your_username@wifi.u-tokyo.ac.jp"
   ```

### 3. Hardware Connection
1. Connect ESP32 to your computer via USB
2. Connect servo motors to their respective pins (pin configuration details to be added)
3. Ensure proper power supply for servo motors

### 4. Building and Uploading
1. Open the project in VSCode with PlatformIO
2. Select the appropriate environment:
   - For WiFi testing: `esp32dev_wifi_test`
   - For main program: `esp32dev_main`
3. Click Upload button or use PlatformIO CLI:
   ```bash
   pio run -t upload
   ```

### 5. Testing Connection
1. Monitor serial output in PlatformIO
2. Check the assigned IP address
3. Run the Python client script:
   ```bash
   python client_test_wifi_TCP.py
   ```

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
