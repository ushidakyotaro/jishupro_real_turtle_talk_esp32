import socket
import struct
import enum
from typing import Optional, Union
from dataclasses import dataclass

class CrushMode(enum.IntEnum):
    SERVO_OFF = 0
    INIT_POSE = 1
    STAY = 2
    SWIM = 3
    RAISE = 4
    EMERGENCY_SURFACE = 5

class WingUpMode(enum.IntEnum):
    RIGHT = 1
    LEFT = 2
    BOTH = 3

@dataclass
class SwimParameters:
    period_sec: float
    wing_deg: float
    max_angle_deg: float
    y_rate: float

class CrushClient:
    def __init__(self, host: str, port: int = 8000):
        self.host = host
        self.port = port
        self.socket = None
        self.connected = False

    def connect(self) -> bool:
        """ESP32に接続する"""
        try:
            self.socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            self.socket.connect((self.host, self.port))
            self.connected = True
            return True
        except Exception as e:
            print(f"Connection error: {e}")
            return False

    def disconnect(self):
        """接続を切断する"""
        if self.socket:
            self.socket.close()
            self.connected = False

    def _send_message(self, message: bytes) -> Optional[bytes]:
        """メッセージを送信し、応答を受け取る"""
        if not self.connected:
            print("Not connected to ESP32")
            return None
        try:
            self.socket.sendall(message)
            return self.socket.recv(1024)
        except Exception as e:
            print(f"Communication error: {e}")
            return None

    def _encode_angle(self, deg: float) -> int:
        """角度を-450~450の整数値に変換"""
        return int(deg * 10)

    def _encode_rate(self, rate: float) -> int:
        """レートを-100~100の整数値に変換"""
        return int(rate * 100)

    def set_mode(self, mode: CrushMode) -> bool:
        """モードを設定する"""
        message = bytes([0x10 | mode.value])
        response = self._send_message(message)
        return response and response[0] & 0x80 == 0

    def set_swim_parameters(self, params: SwimParameters) -> bool:
        """遊泳パラメータを設定する"""
        message = bytearray([0x20])
        message.extend(struct.pack('f', params.period_sec))
        message.extend(struct.pack('h', self._encode_angle(params.wing_deg)))
        message.extend(struct.pack('h', self._encode_angle(params.max_angle_deg)))
        message.append(self._encode_rate(params.y_rate))
        response = self._send_message(message)
        return response and response[0] & 0x80 == 0

    def set_wing(self, mode: WingUpMode, angle: float) -> bool:
        """翼の制御を行う"""
        message = bytearray([0x30 | mode.value])
        message.extend(struct.pack('h', self._encode_angle(angle)))
        response = self._send_message(message)
        return response and response[0] & 0x80 == 0

    def set_mouth(self, open_mouth: bool) -> bool:
        """口の制御を行う"""
        message = bytes([0x40 | int(open_mouth)])
        response = self._send_message(message)
        return response and response[0] & 0x80 == 0

    
    def get_status(self) -> Optional[dict]:
        """ステータスを取得する"""
        message = bytes([0xF0])
        response = self._send_message(message)
        if not response or len(response) < 8:
            return None
    
        mode = response[0]
        current_angle = struct.unpack('h', response[1:3])[0] / 10.0
        error_flags = response[3]
    
        try:
            mode_enum = CrushMode(mode)
        except ValueError:
            print(f"Unknown mode value received: {mode}")
            mode_enum = None  # またはデフォルト値を設定

        return {
            'mode': mode_enum,
            'current_angle': current_angle,
            'wifi_disconnected': bool(error_flags & 0x01),
            'angle_out_of_range': bool(error_flags & 0x02)
        }


# 使用例
def main():
    client = CrushClient("10.100.82.80")
    if not client.connect():
        return

    try:
        # モード設定
        client.set_mode(CrushMode.SWIM)

        # 遊泳パラメータ設定
        params = SwimParameters(
            period_sec=2.0,
            wing_deg=30.0,
            max_angle_deg=45.0,
            y_rate=0.5
        )
        client.set_swim_parameters(params)

        # ステータス取得
        status = client.get_status()
        if status:
            print(f"Current status: {status}")

    finally:
        client.disconnect()

if __name__ == "__main__":
    main()