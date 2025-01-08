import socket
import tkinter as tk
from tkinter import messagebox, ttk
from typing import Optional
from dataclasses import dataclass
import struct
import enum


# CrushMode Enum
class CrushMode(enum.IntEnum):
    SERVO_OFF = 0
    INIT_POSE = 1
    STAY = 2
    SWIM = 3
    RAISE = 4
    EMERGENCY_SURFACE = 5


# SwimCommand Enum
class SwimCommand(enum.IntEnum):
    FORWARD = 0
    TURN_LEFT = 1
    TURN_RIGHT = 2
    RISE_UP = 3
    STAY = 4


# WingUpMode Enum
class WingUpMode(enum.IntEnum):
    RIGHT = 0
    LEFT = 1
    BOTH = 2


# SwimParameters Dataclass
@dataclass
class SwimParameters:
    period_sec: float
    wing_deg: float
    max_angle_deg: float
    y_rate: float
    isBackward: bool = False


# CrushClient Class
class CrushClient:
    def __init__(self, host: str, port: int = 8000):
        self.host = host
        self.port = port
        self.socket = None
        self.connected = False

    def connect(self) -> bool:
        try:
            self.socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            self.socket.connect((self.host, self.port))
            self.connected = True
            return True
        except Exception as e:
            print(f"Connection error: {e}")
            return False

    def disconnect(self):
        if self.socket:
            self.socket.close()
            self.connected = False

    def _send_message(self, message: bytes) -> Optional[bytes]:
        if not self.connected:
            print("Not connected to ESP32")
            return None
        try:
            self.socket.sendall(message)
            return self.socket.recv(1024)
        except Exception as e:
            print(f"Communication error: {e}")
            return None

    def set_mode(self, mode: CrushMode) -> bool:
        message = bytes([0x10 | mode.value])
        response = self._send_message(message)
        return response and response[0] & 0x80 == 0

    def send_swim_command(self, command: SwimCommand) -> bool:
        message = bytes([0x50 | command.value])
        response = self._send_message(message)
        return response and response[0] & 0x80 == 0

    def send_parameters(self, params: SwimParameters) -> bool:
        # パラメータを送信する処理を実装
            # パラメータをバイト列に変換
        period_bytes = struct.pack('f', params.period_sec)
        wing_bytes = struct.pack('h', int(params.wing_deg * 10))
        max_angle_bytes = struct.pack('h', int(params.max_angle_deg * 10))
        y_rate_byte = bytes([int(params.y_rate * 100)])
        backward_byte = bytes([1 if params.isBackward else 0])
        
        # メッセージを作成
        message = bytes([0x20]) + period_bytes + wing_bytes + max_angle_bytes + y_rate_byte + backward_byte
        
        # 送信
        response = self._send_message(message)
        return response and response[0] & 0x80 == 0


    def get_status(self) -> Optional[dict]:
        message = bytes([0xF0])
        response = self._send_message(message)
        if not response or len(response) < 8:
            return None

        mode = response[0]
        current_angle = struct.unpack('h', response[1:3])[0] / 10.0
        error_flags = response[3]

        return {
            "mode": CrushMode(mode),
            "current_angle": current_angle,
            "wifi_disconnected": bool(error_flags & 0x01),
            "angle_out_of_range": bool(error_flags & 0x02),
        }


class RobotControlUI:
    def __init__(self, root):
        self.root = root
        self.root.title("ESP32 亀型ロボット制御")
        self.root.geometry("800x600")
        
        # クライアントの初期化
        self.client = None
        
        # メインフレームの作成
        self.main_frame = ttk.Frame(root)
        self.main_frame.pack(expand=True, fill="both", padx=10, pady=10)
        
        # 左右のフレームを作成
        self.left_frame = ttk.Frame(self.main_frame)
        self.right_frame = ttk.Frame(self.main_frame)
        self.left_frame.pack(side="left", fill="y", padx=(0, 10))
        self.right_frame.pack(side="left", fill="both", expand=True)
        
        # 各セクションの作成
        self.create_status_section()
        self.create_mode_selection()
        self.create_parameter_section()
        
        # 接続設定
        self.select_location()

    def create_status_section(self):
        # ステータス表示部分
        status_frame = ttk.LabelFrame(self.left_frame, text="ステータス", padding=10)
        status_frame.pack(fill="x", pady=(0, 10))
        
        self.status_label = ttk.Label(status_frame, text="未接続")
        self.status_label.pack()

    def create_mode_selection(self):
        # モード選択部分
        mode_frame = ttk.LabelFrame(self.left_frame, text="モード選択", padding=10)
        mode_frame.pack(fill="x", pady=(0, 10))
        
        modes = [
            ("サーボOFF", CrushMode.SERVO_OFF),
            ("初期位置", CrushMode.INIT_POSE),
            ("待機", CrushMode.STAY),
            ("泳ぐ", CrushMode.SWIM),
            ("手を上げる", CrushMode.RAISE)
        ]
        
        for label, mode in modes:
            ttk.Button(mode_frame, text=label,
                      command=lambda m=mode: self.set_mode(m)).pack(fill="x", pady=2)
        
        # 緊急停止ボタン
        ttk.Button(mode_frame, text="緊急停止",
                  command=lambda: self.set_mode(CrushMode.EMERGENCY_SURFACE),
                  style="Emergency.TButton").pack(fill="x", pady=(10, 2))

    def create_parameter_section(self):
        # パラメータ設定部分
        self.parameter_frame = ttk.LabelFrame(self.right_frame, text="パラメータ設定", padding=10)
        self.parameter_frame.pack(fill="both", expand=True)
        
        # 各モード用のパラメータウィジェットを作成
        self.create_swim_parameters()
        
    def create_swim_parameters(self):
        # 既存のウィジェットをクリア
        for widget in self.parameter_frame.winfo_children():
            widget.destroy()
            
        # 周期設定
        ttk.Label(self.parameter_frame, text="周期 (秒)").pack()
        self.period_scale = ttk.Scale(self.parameter_frame, from_=0.5, to=3.0,
                                    orient="horizontal")
        self.period_scale.pack(fill="x")
        
        # 羽ばたき角度
        ttk.Label(self.parameter_frame, text="羽ばたき角度 (度)").pack()
        self.wing_scale = ttk.Scale(self.parameter_frame, from_=0, to=90,
                                  orient="horizontal")
        self.wing_scale.pack(fill="x")
        
        # 最大角度
        ttk.Label(self.parameter_frame, text="最大角度 (度)").pack()
        self.max_angle_scale = ttk.Scale(self.parameter_frame, from_=0, to=45,
                                       orient="horizontal")
        self.max_angle_scale.pack(fill="x")
        
        # 左右バランス
        ttk.Label(self.parameter_frame, text="左右バランス").pack()
        self.balance_scale = ttk.Scale(self.parameter_frame, from_=-1.0, to=1.0,
                                     orient="horizontal")
        self.balance_scale.pack(fill="x")
        
        # 前進/後退切り替え
        self.backward_var = tk.BooleanVar()
        ttk.Checkbutton(self.parameter_frame, text="後退モード",
                       variable=self.backward_var).pack()
        
        # 適用ボタン
        ttk.Button(self.parameter_frame, text="パラメータを適用",
                  command=self.apply_parameters).pack(pady=10)

    def apply_parameters(self):
        params = SwimParameters(
            period_sec=self.period_scale.get(),
            wing_deg=self.wing_scale.get(),
            max_angle_deg=self.max_angle_scale.get(),
            y_rate=self.balance_scale.get(),
            isBackward=self.backward_var.get()
        )
        if self.client:
            self.client.send_parameters(params)

    def set_mode(self, mode: CrushMode):
        if not self.client or not self.client.connected:
            messagebox.showerror("エラー", "ESP32に接続されていません")
            return
        
        if self.client.set_mode(mode):
            self.update_status()
            # モードに応じたパラメータ設定画面を表示
            if mode == CrushMode.SWIM:
                self.create_swim_parameters()
        else:
            messagebox.showerror("エラー", f"モード {mode.name} の設定に失敗しました")

    def update_status(self):
        if not self.client or not self.client.connected:
            self.status_label.config(text="未接続")
            return
            
        status = self.client.get_status()
        if status:
            self.status_label.config(
                text=(
                    f"現在のモード: {status['mode'].name}\n"
                    f"角度: {status['current_angle']}\xb0\n"
                    f"WiFi切断: {status['wifi_disconnected']}\n"
                    f"角度範囲外: {status['angle_out_of_range']}"
                )
            )
        else:
            self.status_label.config(text="ステータスの取得に失敗しました")

    def select_location(self):
        location_window = tk.Toplevel(self.root)
        location_window.title("作業場所の選択")
        location_window.geometry("300x150")
        
        def set_home():
            self.client = CrushClient("10.1.100.158")
            location_window.destroy()
            self.connect_to_esp32()
            
        def set_school():
            self.client = CrushClient("10.100.82.80")
            location_window.destroy()
            self.connect_to_esp32()
            
        ttk.Label(location_window, text="作業場所を選んでください").pack(pady=10)
        ttk.Button(location_window, text="家", command=set_home).pack(pady=5)
        ttk.Button(location_window, text="学校", command=set_school).pack(pady=5)

    def connect_to_esp32(self):
        if self.client.connect():
            messagebox.showinfo("接続成功", "ESP32に接続しました")
            self.start_status_update()
        else:
            retry = messagebox.askretrycancel("接続エラー", "ESP32に接続できませんでした。再試行しますか？")
            if retry:
                self.connect_to_esp32()
            else:
                self.root.quit()

    def start_status_update(self):
        def update():
            self.update_status()
            self.root.after(2000, update)
        update()


def main():
    root = tk.Tk()
    app = RobotControlUI(root)
    root.mainloop()


if __name__ == "__main__":
    main()
