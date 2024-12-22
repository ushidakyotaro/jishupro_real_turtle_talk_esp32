# Sea Turtle Character (Crush) Robot and Interactive Turtle Talk Simulation

This project aims to replicate the sea turtle character and turtle talk experience from Tokyo DisneySea, developed as an independent project (2024) at the Department of Mechano-Informatics, The University of Tokyo.

## Project Overview
- Recreation of the Crush character from Finding Nemo/Tokyo DisneySea
- Interactive communication system using ESP32
- Servo motor control for realistic movement
- TCP/IP communication for real-time control

## Project Structure
```plaintext
jishupro_esp32/
├── esp32/          # ESP32 firmware and tests
│   ├── include/    # Header files
│   ├── src/        # Main source code
│   ├── test/       # Test code
│   └── README.md   # ESP32-specific documentation
├── client/         # PC-side application
│   └── src/        # Client source code
└── README.md       # This file
```

## Quick Start Guide

### 1. Clone the Repository
```bash
git clone https://github.com/ushidakyotaro/jishupro_real_turtle_talk_esp32.git
cd jishupro_real_turtle_talk_esp32
```

### 2. ESP32 Development
For ESP32 firmware development and testing:
1. Open VSCode
2. Install PlatformIO Extension
3. Open the esp32 directory in VSCode:
   ```bash
   code esp32
   ```
4. See [esp32/README.md](esp32/README.md) for detailed instructions

### 3. Client Development
For PC-side application:
1. Install Python requirements:
   ```bash
   cd client
   python -m pip install -r requirements.txt
   ```
2. Run the test client:
   ```bash
   python client_test_wifi_TCP.py
   ```

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

## License
This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## Contact
For questions about this project, please contact:
- Please create an issue on our [GitHub repository](https://github.com/ushidakyotaro/jishupro_real_turtle_talk_esp32/issues)
- All bug reports, feature requests, and general questions are welcome