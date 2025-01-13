import pygame
import tkinter as tk
from tkinter import messagebox
import os

# 音声ファイルと文字起こしテキストのリスト
audio_files = [
    "first.wav", "prelude_2.wav", "prelude_3.wav", "prelude_4.wav",
    "prelude_5.wav", "prelude_6.wav", "prelude_7.wav", "prelude_8.wav",
    "prelude_9.wav", "prelude_10.wav", "prelude_11.wav",
    "final_1.wav", "final_2.wav", "final_4.wav", "final_5.wav"
]

transcripts = [
    "おーー、さーいこーう！",
    "お、こいつはすげー！こーんなにたくさん、人がいるぜ！",
    "よお、みんな。こんにちは",
    "お前たち、最高だぜ！",
    "おいおいおい、お前ら元気ないなー",
    "自主プロの製作で徹夜したのかな？",
    "ハハハ、いやー、実はさ、俺の息子のスクワートが人間について、知りたがってるんだ。",
    "だからさ、今日はみんなと話すのを、すげー楽しみにしてたんだ！",
    "俺に協力してくれるやつはヒレをあげてくれ",
    "お前たち、最高だぜ！",
    "じゃあ、ガイドさん、俺に質問があるやつを一人、選んでくれ",
    "おっと、スクワートが俺のことを呼んでいるみたいだ。",
    "ハハハ、今日はお前たちと会えて楽しかったぜ。",
    "この後はガイドさんたちがロボットの実装について説明してくれるみたいだから、しっかり話を聞いてやってくれ。",
    "それじゃあ、またな！"
]

# 初期設定
global current_index
current_index = 0
pygame.init()

# 音声を再生する関数
def play_audio(index):
    if 0 <= index < len(audio_files):
        pygame.mixer.music.load(audio_files[index])
        pygame.mixer.music.play()
        show_transcript(transcripts[index])

# テキストを表示するウィンドウを作成する関数
def show_transcript(text):
    window = tk.Tk()
    window.title("Transcript")
    window.geometry("800x400")
    label = tk.Label(window, text=text, font=("Arial", 24), wraplength=700, justify="center")
    label.pack(expand=True, fill="both")
    window.mainloop()

# GUI操作ウィンドウを作成する関数
def create_control_window():
    def play_previous():
        global current_index
        if current_index > 0:
            current_index -= 1
            play_audio(current_index)

    def play_current():
        global current_index
        play_audio(current_index)

    def play_next():
        global current_index
        if current_index < len(audio_files) - 1:
            current_index += 1
            play_audio(current_index)

    control_window = tk.Tk()
    control_window.title("Audio Control")
    control_window.geometry("400x200")

    btn_previous = tk.Button(control_window, text="Previous", font=("Arial", 16), command=play_previous)
    btn_previous.pack(fill="x", padx=20, pady=10)

    btn_current = tk.Button(control_window, text="Play Current", font=("Arial", 16), command=play_current)
    btn_current.pack(fill="x", padx=20, pady=10)

    btn_next = tk.Button(control_window, text="Next", font=("Arial", 16), command=play_next)
    btn_next.pack(fill="x", padx=20, pady=10)

    control_window.mainloop()

# メイン処理
if __name__ == "__main__":
    # 音声ファイルの存在確認
    for file in audio_files:
        if not os.path.exists(file):
            messagebox.showerror("Error", f"Audio file '{file}' not found!")
            exit()

    # コントロールウィンドウを作成
    create_control_window()

    pygame.quit()
