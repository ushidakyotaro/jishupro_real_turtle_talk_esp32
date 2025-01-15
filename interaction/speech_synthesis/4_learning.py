# 上でつけたモデル名を入力。学習を途中からする場合はきちんとモデルが保存されているフォルダ名を入力。
model_name = "newCrush"


import yaml
from gradio_tabs.train import get_path

paths = get_path(model_name)
dataset_path = str(paths.dataset_path)
config_path = str(paths.config_path)

with open("default_config.yml", "r", encoding="utf-8") as f:
    yml_data = yaml.safe_load(f)
yml_data["model_name"] = model_name
with open("config.yml", "w", encoding="utf-8") as f:
    yaml.dump(yml_data, f, allow_unicode=True)



# 日本語特化版を「使う」場合
!python train_ms_jp_extra.py --config {config_path} --model {dataset_path} --assets_root {assets_root}