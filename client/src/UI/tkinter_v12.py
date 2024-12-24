import socket
import tkinter as tk
from tkinter import messagebox, Toplevel
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


# SwimParameters Dataclass
@dataclass
class SwimParameters:
    period_sec: float
    wing_deg: float
    max_angle_deg: float
    y_rate: float


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


# グローバルクライアント
client = CrushClient("10.100.82.80")  # 学校用のIPアドレス


def connect_to_esp32():
    import time

    start_time = time.time()
    while time.time() - start_time < 10:
        if client.connect():
            messagebox.showinfo("接続成功", "ESP32に接続しました")
            return
        time.sleep(1)  # 1秒待機して再試行

    messagebox.showerror("接続失敗", "10秒以内にESP32に接続できませんでした")
    root.quit()
    if not client.connect():
        retry = messagebox.askretrycancel("接続エラー", "ESP32に接続できませんでした。再試行しますか？")
        if not retry:
            root.quit()
    else:
        messagebox.showinfo("接続成功", "ESP32に接続しました")
    if not client.connect():
        retry = messagebox.askretrycancel("接続エラー", "ESP32に接続できませんでした。再試行しますか？")
        if not retry:
            root.quit()
    else:
        messagebox.showinfo("接続成功", "ESP32に接続しました")


def close_connection():
    try:
        if client:
            client.set_mode(CrushMode.INIT_POSE)
            time.sleep(1)  # 1秒待機してモード変更を反映
            client.set_mode(CrushMode.SERVO_OFF)
    except Exception as e:
        print(f"モード変更エラー: {e}")
    finally:
        client.disconnect()
        messagebox.showinfo("接続終了", "ESP32との接続を終了しました")
        root.quit()
    client.disconnect()
    messagebox.showinfo("接続終了", "ESP32との接続を終了しました")
    root.quit()


def update_status():
    """ステータスを取得してラベルを更新する"""
    status = client.get_status()
    if status:
        status_label.config(
            text=(
                f"現在のモード: {status['mode'].name}\n"
                f"角度: {status['current_angle']}\xb0\n"
                f"WiFi切断: {status['wifi_disconnected']}\n"
                f"角度範囲外: {status['angle_out_of_range']}"
            )
        )
    else:
        status_label.config(text="ステータスの取得に失敗しました")


def set_mode(mode: CrushMode):
    if client.set_mode(mode):
        update_status()
        if mode == CrushMode.SWIM:
            open_swim_window()
    else:
        messagebox.showerror("エラー", f"モード {mode.name} の設定に失敗しました")


def open_swim_window():
    swim_window = Toplevel(root)
    swim_window.title("SWIMモード")
    swim_window.geometry("400x400")

    commands = [
        ("前進", SwimCommand.FORWARD),
        ("左旋回", SwimCommand.TURN_LEFT),
        ("右旋回", SwimCommand.TURN_RIGHT),
        ("上昇", SwimCommand.RISE_UP),
        ("待機", SwimCommand.STAY),
    ]

    for label, command in commands:
        tk.Button(
            swim_window,
            text=label,
            width=20,
            command=lambda c=command: send_swim_command(c),
        ).pack(pady=5)

    tk.Button(
        swim_window,
        text="緊急停止",
        command=lambda: set_mode(CrushMode.EMERGENCY_SURFACE),
        bg="red",
        fg="white",
        width=20,
    ).pack(pady=20)

    tk.Button(swim_window, text="戻る", command=swim_window.destroy, width=15).pack(pady=10)


def send_swim_command(command: SwimCommand):
    if client.send_swim_command(command):
        print(f"{command.name} コマンド送信成功")
    else:
        messagebox.showerror("エラー", f"{command.name} コマンド送信に失敗しました")


def select_location():
    def set_home():
        global client
        client = CrushClient("192.168.11.9")  # 家のIPアドレス
        location_window.destroy()
        connect_to_esp32()

    def set_school():
        global client
        client = CrushClient("10.100.82.80")  # 学校のIPアドレス
        location_window.destroy()
        connect_to_esp32()

    location_window = tk.Tk()
    location_window.title("作業場所の選択")
    location_window.geometry("300x150")

    tk.Label(location_window, text="作業場所を選んでください", font=("Helvetica", 12)).pack(pady=10)

    tk.Button(location_window, text="家", command=set_home, width=15).pack(pady=5)
    tk.Button(location_window, text="学校", command=set_school, width=15).pack(pady=5)

    location_window.mainloop()

def select_location():
    def set_home():
        global client
        client = CrushClient("192.168.11.9")  # 家のIPアドレス
        location_window.destroy()

    def set_school():
        global client
        client = CrushClient("10.100.82.80")  # 学校のIPアドレス
        location_window.destroy()

    location_window = tk.Tk()
    location_window.title("作業場所の選択")
    location_window.geometry("300x150")

    tk.Label(location_window, text="作業場所を選んでください", font=("Helvetica", 12)).pack(pady=10)

    tk.Button(location_window, text="家", command=set_home, width=15).pack(pady=5)
    tk.Button(location_window, text="学校", command=set_school, width=15).pack(pady=5)

    location_window.mainloop()

select_location()

root = tk.Tk()
root.title("ESP32 亀型ロボット制御")
root.geometry("500x600")

connect_to_esp32()

status_label = tk.Label(root, text="現在のステータス: 未収得", font=("Helvetica", 12))
status_label.pack(pady=10)

tk.Label(root, text="モード選択", font=("Helvetica", 14)).pack(pady=10)

modes = [
    ("オフ", CrushMode.SERVO_OFF),
    ("初期位置", CrushMode.INIT_POSE),
    ("待機", CrushMode.STAY),
    ("泳ぐ", CrushMode.SWIM),
    ("手を上げる", CrushMode.RAISE),
]

for mode_label, mode_enum in modes:
    tk.Button(root, text=mode_label, width=20, command=lambda m=mode_enum: set_mode(m)).pack(pady=5)

tk.Button(
    root,
    text="緊急停止",
    command=lambda: set_mode(CrushMode.EMERGENCY_SURFACE),
    bg="red",
    fg="white",
    width=20,
).pack(pady=20)

tk.Button(root, text="終了", command=close_connection, width=15).pack(pady=10)

# ステータスを定期的に更新
def periodic_status_update():
    update_status()
    root.after(2000, periodic_status_update)  # 2秒ごとに更新

periodic_status_update()

root.mainloop()
