import socket
import tkinter as tk
from tkinter import messagebox, ttk
from typing import Optional
from dataclasses import dataclass
import struct
import enum
import time


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
        self.connect_timeout = 5.0  # 接続タイムアウト: 5秒
        self.operation_timeout = 2.0  # 操作タイムアウト: 2秒

    def connect(self) -> bool:
        self.disconnect()
        try:
            self.socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            self.socket.connect((self.host, self.port))
            self.socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
            # 接続実行
            self.socket.settimeout(self.connect_timeout)
            self.connected = True
            return True
        except socket.timeout:
            print("Connection timeout")
            self.cleanup()
            return False
        except ConnectionRefusedError:
            print("Connection refused - server might not be ready")
            self.cleanup()
            return False
        except Exception as e:
            print(f"Connection error: {e}")
            self.cleanup()
            return False

    def disconnect(self):
        if self.socket:
            try:
                self.socket.shutdown(socket.SHUT_RDWR)
            except:
                pass
            try:
                self.socket.close()
            except:
                pass
            self.socket = None
        self.connected = False

    def cleanup(self):
        """リソースのクリーンアップ"""
        self.disconnect()
        time.sleep(0.1)  # 短い待機時間を設定

    def _send_message(self, message: bytes) -> Optional[bytes]:
        if not self.connected or not self.socket:
            print("Not connected to ESP32")
            return None

        try:
            self.socket.settimeout(self.operation_timeout)
            self.socket.sendall(message)
            response = self.socket.recv(1024)
            return response
            
        except socket.timeout:
            print("Operation timeout")
            self.cleanup()
            return None
        except ConnectionResetError:
            print("Connection reset by peer")
            self.cleanup()
            return None
        except Exception as e:
            print(f"Communication error: {e}")
            self.cleanup()
            return None

    def set_mode(self, mode: CrushMode) -> bool:
        message = bytes([0x10 | mode.value])
        response = self._send_message(message)
        if not response:
            self.cleanup()
            return False
        return response[0] & 0x80 == 0

    def send_swim_command(self, command: SwimCommand) -> bool:
        message = bytes([0x50 | command.value])
        response = self._send_message(message)
        return response and response[0] & 0x80 == 0

    def send_parameters(self, params: SwimParameters) -> bool:
        period_bytes = struct.pack('f', params.period_sec)
        wing_bytes = struct.pack('h', int(params.wing_deg * 10))
        max_angle_bytes = struct.pack('h', int(params.max_angle_deg * 10))
        y_rate_byte = bytes([int(params.y_rate * 100)])
        backward_byte = bytes([1 if params.isBackward else 0])
        
        message = bytes([0x20]) + period_bytes + wing_bytes + max_angle_bytes + y_rate_byte + backward_byte
        response = self._send_message(message)
        if not response:
            self.cleanup()
            return False
        return response[0] & 0x80 == 0

    def get_status(self) -> Optional[dict]:
        message = bytes([0xF0])
        response = self._send_message(message)
        if not response or len(response) < 8:
            return None

        try:
            mode = response[0]
            current_angle = struct.unpack('h', response[1:3])[0] / 10.0
            error_flags = response[3]

            return {
                "mode": CrushMode(mode),
                "current_angle": current_angle,
                "wifi_disconnected": bool(error_flags & 0x01),
                "angle_out_of_range": bool(error_flags & 0x02),
            }
        except Exception as e:
            print(f"Status parsing error: {e}")
            return None


class RobotControlUI:
    def __init__(self, root):
        self.root = root
        self.root.title("ESP32 亀型ロボット制御")
        self.root.geometry("800x600")
        
        # クライアントの初期化
        self.client = None
        self.connection_retry_count = 0
        self.max_connection_retries = 3
        
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

        # 接続状態管理用の変数
        self.is_manually_disconnected = False
        
        # 接続管理セクションの作成
        self.create_connection_section()

    def create_connection_section(self):
        # 接続管理ボタンを左フレームに追加
        conn_frame = ttk.LabelFrame(self.left_frame, text="接続管理", padding=10)
        conn_frame.pack(fill="x", pady=(0, 10))
        
        self.connect_button = ttk.Button(conn_frame, text="接続", command=self.manual_connect)
        self.connect_button.pack(fill="x", pady=2)
        
        self.disconnect_button = ttk.Button(conn_frame, text="切断", command=self.manual_disconnect)
        self.disconnect_button.pack(fill="x", pady=2)
        
        # ボタンの初期状態を設定
        self.disconnect_button.state(['disabled'])

    def manual_connect(self):
        if self.is_manually_disconnected:
            self.is_manually_disconnected = False
            self.connect_to_esp32()
            if self.client and self.client.connected:
                self.connect_button.state(['disabled'])
                self.disconnect_button.state(['!disabled'])
                messagebox.showinfo("接続", "ESP32に再接続しました")

    def manual_disconnect(self):
        if self.client and self.client.connected:
            self.client.disconnect()
            self.is_manually_disconnected = True
            self.connect_button.state(['!disabled'])
            self.disconnect_button.state(['disabled'])
            self.status_label.config(text="手動切断")
            messagebox.showinfo("切断", "ESP32との接続を切断しました")

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
            if not self.is_manually_disconnected:
                self.status_label.config(text="未接続")
                self.connect_button.state(['!disabled'])
                self.disconnect_button.state(['disabled'])
                self.handle_disconnection()
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
            self.status_label.config(text="ステータスの取得に失敗")
            if not self.is_manually_disconnected:
                self.handle_disconnection()

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
        if self.connection_retry_count >= self.max_connection_retries:
            messagebox.showerror("接続エラー", "接続の最大試行回数を超えました。\nプログラムを再起動してください。")
            self.root.quit()
            return

        if self.client.connect():
            self.connection_retry_count = 0
            self.connect_button.state(['disabled'])
            self.disconnect_button.state(['!disabled'])
            messagebox.showinfo("接続成功", "ESP32に接続しました")
            self.start_status_update()
        else:
            self.connection_retry_count += 1
            retry = messagebox.askretrycancel(
                "接続エラー", 
                f"ESP32に接続できませんでした。\n試行回数: {self.connection_retry_count}/{self.max_connection_retries}\n再試行しますか？"
            )
            if retry:
                self.root.after(1000, self.connect_to_esp32)
            else:
                self.root.quit()


    def start_status_update(self):
        def update():
            self.update_status()
            self.root.after(2000, update)
        update()

    def handle_disconnection(self):
        """接続が切れた際の処理"""
        if self.client:
            self.client.cleanup()
            retry = messagebox.askretrycancel("接続エラー", "接続が切れました。再接続しますか？")
            if retry:
                self.connection_retry_count = 0
                self.connect_to_esp32()
            else:
                self.root.quit()

    def on_closing(self):
        """ウィンドウが閉じられる際の処理"""
        if self.client:
            self.client.disconnect()
        self.root.destroy()


def main():
    root = tk.Tk()
    app = RobotControlUI(root)
    root.protocol("WM_DELETE_WINDOW", app.on_closing)  # 終了時の処理を設定
    root.mainloop()


if __name__ == "__main__":
    main()
