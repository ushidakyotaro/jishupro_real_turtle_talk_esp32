!pip install --upgrade pip
!pip install onnx onnxruntime onnxsim

import os

print("現在の作業ディレクトリ:", os.getcwd())

if os.getcwd() != "/content/Style-Bert-VITS2":
    %cd /content/Style-Bert-VITS2/
    print("移動後の作業ディレクトリ:", os.getcwd())

!git checkout dev

!git pull origin dev

if os.path.exists("convert_onnx.py"):
    print("convert_onnx.py が存在します。")
else:
    print("convert_onnx.py が見つかりません。リポジトリを再度確認してください。")

model_name = "newCrush"  # 実際のモデル名に置き換えてください

model_path = f"/content/drive/MyDrive/Style-Bert-VITS2/model_assets/{model_name}/newCrush_e100_s1300.safetensors"  # 実際のファイル名に置き換えてください

if os.path.exists(model_path):
    print(f"モデルファイルが見つかりました: {model_path}")
else:
    print(f"モデルファイルが見つかりません: {model_path}")

!python convert_onnx.py --model "{model_path}" --force-convert

!ls -lh /content/drive/MyDrive/Style-Bert-VITS2/model_assets/{model_name}/

onnx_path = f"/content/drive/MyDrive/Style-Bert-VITS2/model_assets/{model_name}/newCrush_e100_s1300.onnx"  # 上で入力したsafetensorsのファイル名と同じ名前に置き換えてください（上の名前が「aaa.safetensors」なら「aaa.onnx」にします）

if os.path.exists(onnx_path):
    print(f"ONNXファイルが正常に作成されました: {onnx_path}")
else:
    print("ONNXファイルの作成に失敗しました。エラーメッセージを確認してください。")