import io
import json
import soundfile
import requests
import google.generativeai as genai
from playsound import playsound
import tkinter as tk
from os.path import abspath, exists
import threading


from dotenv import load_dotenv
import os

# .envファイルを読み込む
load_dotenv()

# 環境変数を取得
gemini_api_key = os.getenv('GEMINI_API')

class AivisAdapter:
    # ... (既存のAivisAdapterのコード)
    def __init__(self):
        # APIサーバーのエンドポイントURL
        self.URL = "http://127.0.0.1:10101"
        # 話者ID (話させたい音声モデルidに変更してください)
        self.speaker = 1196797856

    def save_voice(self, text: str, output_filename: str = "output.wav"):
        params = {"text": text, "speaker": self.speaker}
        query_response = requests.post(f"{self.URL}/audio_query", params=params).json()

        audio_response = requests.post(
            f"{self.URL}/synthesis",
            params={"speaker": self.speaker},
            headers={"accept": "audio/wav", "Content-Type": "application/json"},
            data=json.dumps(query_response),
        )

        with io.BytesIO(audio_response.content) as audio_stream:
            data, rate = soundfile.read(audio_stream)
            soundfile.write(output_filename, data, rate)



# 自分の Gemini API キーをここに貼り付ける
genai.configure(api_key=gemini_api_key)

def get_llm_response(user_input, context):
    model = genai.GenerativeModel('gemini-pro')
    
    # 過去のコンテキストと今回の入力を合わせて送信
    setting = ""  # 必要なら設定を定義
    #combined_input = f"{context}\nUser: {user_input}"
    combined_input = f"{setting}\n{context}\nUser: {user_input}"

    # 生成されたコンテンツを取得
    response = model.generate_content(combined_input)

    # 応答内容を取得
    return response.text

def display_in_window_with_audio(output_filename, response_text):
    """音声再生中にウィンドウを表示し、音声終了後にウィンドウを閉じる"""
    window = tk.Tk()
    window.title("AI Response")
    window.geometry("800x400")

    label = tk.Label(
        window, 
        text=response_text, 
        font=("Arial", 24), 
        wraplength=700, 
        justify="center"
    )
    label.pack(expand=True)

    # 音声再生を別スレッドで実行
    def play_audio():
        absolute_path = abspath(output_filename)
        if exists(absolute_path):
            playsound(absolute_path)
        else:
            print(f"Error: {absolute_path} not found.")
        # 音声再生が終了したらウィンドウを閉じる
        window.after(0,window.destroy)

    # 音声再生スレッドを開始
    audio_thread = threading.Thread(target=play_audio)
    audio_thread.start()

    # ウィンドウのイベントループを開始
    window.mainloop()

    # 再生スレッドが終了するのを待機
    audio_thread.join()
    


def main():
    adapter = AivisAdapter()
    context = "\n今回はディズニーシーのアトラクションの一つであるタートルトークのクラッシュになりきって話して欲しいです。今から書く手順にしたがって会話を進めてください。①まず、ユーザが話しかけてきたら、「やあ、こんにちは。名前はなんていうんだい」と言ってください。②次にさっき聞いた名前を()の中に入れて、「()かぁ。よろしくな！質問はなんだい？」と言う。(名前)にはさっき聞いた名前をいれてください。③ウミガメのクラッシュになりきって、質問に答える。その後、それに関連した質問を相手に投げかける。④相手の返答に答える。このときできるだけ、簡潔になるようにする。また、相手に質問はこれ以上投げかけない。セリフの最後に「お前たち、最高だぜ！」をつける。注意点として返答を送るときにセリフの前に「クラッシュ」や「:」などの無駄な単語は省き、セリフ単体を送ってください。セリフはできるだけ、簡潔にお願いします。\nわかったぜ！じゃあさっそくはじめよう！"  # 過去のコンテキストを保存する変数
    output_filename = "output.wav"

    while True: 
        user_input = input(">")
        llm_response = get_llm_response(user_input, context)
        print(f"Response: {llm_response}")

        adapter.save_voice(llm_response, output_filename)
        
        display_in_window_with_audio(output_filename, llm_response)
        

        # 応答をコンテキストに追加
        context += f"\n{user_input}\n {llm_response}"

        

if __name__ == "__main__":
    main()