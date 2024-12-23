import socket
import time

def connect_to_esp32():
    # ESP32のIPアドレス（シリアルモニタで表示されたIPアドレスに変更してください）
    esp32_ip = "10.100.82.80"  # ← ここを実際のESP32のIPアドレスに変更
    port = 8000

    try:
        # ESP32に接続
        client = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        client.connect((esp32_ip, port))
        print(f"ESP32 ({esp32_ip}:{port}) に接続しました")
        
        while True:
            # 送信する文字列（'quit'で終了）
            message = input("送信メッセージを入力してください（終了する場合は'quit'）: ")
            
            # メッセージを送信
            client.sendall(message.encode())
            
            # ESP32からの応答を受信
            response = client.recv(4096)
            print("ESP32からの応答:", response.decode())
            
            # 'quit'が入力されたら終了
            if message.lower() == 'quit':
                break
                
    except ConnectionRefusedError:
        print("接続が拒否されました。ESP32のIPアドレスとポート番号を確認してください。")
    except Exception as e:
        print(f"エラーが発生しました: {e}")
    finally:
        client.close()
        print("接続を終了しました")

if __name__ == "__main__":
    connect_to_esp32()