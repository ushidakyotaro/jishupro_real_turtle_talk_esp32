import os

os.environ["PATH"] += ":/root/.cargo/bin"

!curl -LsSf https://astral.sh/uv/install.sh | sh
!git clone https://github.com/litagin02/Style-Bert-VITS2.git
%cd Style-Bert-VITS2/
!uv pip install --system -r requirements-colab.txt
!python initialize.py --skip_default_models