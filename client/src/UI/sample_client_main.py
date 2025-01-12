# robot_control_lib.py
import enum
from dataclasses import dataclass
import struct
import socket
from typing import Optional

class RobotMode(enum.IntEnum):
    SERVO_OFF = 0
    INIT_POSE = 1
    MOTION_1 = 2
    MOTION_2 = 3
    MOTION_3 = 4
    EMERGENCY = 5

@dataclass
class MotionParameters:
    param1: float  # 時間パラメータ (例: 周期)
    param2: float  # 角度パラメータ1
    param3: float  # 角度パラメータ2
    param4: float  # 調整パラメータ
    flag1: bool = False  # 制御フラグ

class RobotClient:
    def __init__(self, host: str, port: int = 8000):
        self.host = host
        self.port = port
        self.socket = None
        self.connected = False
        self.connect_timeout = 5.0
        self.operation_timeout = 2.0

    # 接続管理メソッド
    def connect(self) -> bool:
        # (既存のconnect実装)
        pass

    def disconnect(self):
        # (既存のdisconnect実装)
        pass

    # 制御メソッド
    def set_mode(self, mode: RobotMode) -> bool:
        message = bytes([0x10 | mode.value])
        response = self._send_message(message)
        return response and response[0] & 0x80 == 0

    def send_parameters(self, params: MotionParameters) -> bool:
        param1_bytes = struct.pack('f', params.param1)
        param2_bytes = struct.pack('h', int(params.param2 * 10))
        param3_bytes = struct.pack('h', int(params.param3 * 10))
        param4_byte = struct.pack('b', int(params.param4 * 100))
        flag1_byte = bytes([1 if params.flag1 else 0])
        
        message = bytes([0x20]) + param1_bytes + param2_bytes + param3_bytes + param4_byte + flag1_byte
        response = self._send_message(message)
        return response and response[0] & 0x80 == 0

    def get_status(self) -> Optional[dict]:
        # (既存のget_status実装)
        pass