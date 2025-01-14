import io
import json
import soundfile
import requests
import google.generativeai as genai
import tkinter as tk
from os.path import abspath, exists, normpath
import threading
import speech_recognition as sr
from pydub import AudioSegment
from pydub.playback import play

# from dotenv import load_dotenv
# import os
# load_dotenv()
# gemini_api_key = os.getenv('GEMINI_API')

class AivisAdapter:
    def __init__(self):
        self.URL = "http://127.0.0.1:10101"
        self.speaker = 1196797856

    def save_voice(self, text: str, output_filename: str = 'output.wav'):
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

# Google Generative AI API設定
# genai.configure(api_key=gemini_api_key)
genai.configure(api_key="AIzaSyA9kv__y4YJ760ejTnOyzQIEzMLjxALH04")

def get_llm_response(user_input, context):
    """AIからの応答を取得"""
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

    def play_audio():
        if exists(output_filename):
            try:
                # pydubで音声を再生
                audio = AudioSegment.from_file(output_filename)
                play(audio)
            except Exception as e:
                print(f"Error playing audio: {e}")
        else:
            print(f"Error: {output_filename} not found.")
        window.after(0, window.destroy)

    audio_thread = threading.Thread(target=play_audio)
    audio_thread.start()
    window.mainloop()
    audio_thread.join()

def get_user_input():
    """音声認識でユーザー入力を取得"""
    recognizer = sr.Recognizer()
    with sr.Microphone() as source:
        print("話してください...")
        try:
            audio = recognizer.listen(source, timeout=5)
            text = recognizer.recognize_google(audio, language="ja-JP")  # 日本語認識
            print(f"認識結果: {text}")
            return text
        except sr.WaitTimeoutError:
            print("音声が検出されませんでした。")
            return "音声が検出されませんでした。"
        except sr.UnknownValueError:
            print("音声を認識できませんでした。")
            return "音声を認識できませんでした。"
        except Exception as e:
            print(f"エラー: {e}")
            return "エラーが発生しました。"

def main():
    adapter = AivisAdapter()
    context = """
    # Concept
    ディズニーシーのアトラクションの一つであるタートル・トークのクラッシュの役として、お客さんと会話する。

    # Steps
    1. ユーザが話しかけてきたら、「やあ、こんにちは。名前はなんていうんだい」と言う。
    2. 次に聞いた名前を()の中に入れて、「()かぁ。渋い名前だなぁ。よろしくな！質問はなんだい？」と言う。()にはさっき聞いた名前をいれる。
    3. ウミガメのクラッシュになりきって、質問に答える。その後、それに関連した質問を相手に投げかける。相手の返答が来るまで待つ。
    4. 相手の返答に簡潔に答えるまた、相手に質問はこれ以上投げかけない。セリフの最後に「。お前たち、最高だぜ！！」をつける。相手の返答が来るまで待つ。
    5. 相手が返答したら、一言付け加えた後、どのような返答であろうと必ず以下のように答える。「おっと、スクワートが俺のことを呼んでいるみたいだ。ハハハ、今日はお前たちと会えて楽しかったぜ。それじゃあ、またな！」と言う。
    
    # caution
    - セリフのみを簡潔に送信する。返答を送るときにセリフの前に「クラッシュ」、「:」、「User」、「AI」などの無駄な単語は省き、セリフだけを送ってください。お願いします。絶対に守ってください
    - Userの部分は生成せず、クラッシュの発言のみ生成する。
    - 日本語で会話する。

    # example
    User:こんにちは
    AI:やあ、こんにちは。名前はなんていうんだい
    User:しのぶです
    AI:しのぶかぁ。渋い名前だなぁ。よろしくな！質問はなんだい？
    User:クラッシュの体重はどれくらいですか？
    AI:俺の体重かい？正確には知らないんだけど、200キロ以上はあると思うぜ。しのぶはどのくらいなんだ？
    User:60キロくらいです
    AI:60キロしかないのか。まだまだ子供のウミガメじゃないか。お前たち、最高だぜ！！
    User:うおー
    AI:おっと、スクワートが俺のことを呼んでいるみたいだ。ハハハ、今日はお前たちと会えて楽しかったぜ。それじゃあ、またな！
    """
    output_filename = 'output.wav'

    while True:
        user_input = get_user_input()  # 音声認識で入力を取得
        if user_input in ["音声が検出されませんでした。", "音声を認識できませんでした。", "エラーが発生しました。"]:
            continue  # 音声認識に失敗した場合はスキップ

        llm_response = get_llm_response(user_input, context)
        print(f"Response: {llm_response}")

        adapter.save_voice(llm_response, output_filename)
        display_in_window_with_audio(output_filename, llm_response)

        context += f"\n {user_input}\n {llm_response}"

if __name__ == "__main__":
    main()
