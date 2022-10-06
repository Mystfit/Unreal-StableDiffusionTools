import os, sys, subprocess
import unreal

pythonpath = unreal.Paths().make_standard_filename(os.path.join("..", "ThirdParty", "Python3", "Win64", "python.exe"))

pip_dependencies = [
    "torch torchvision torchaudio --extra-index-url https://download.pytorch.org/whl/cu116",
    "diffusers"
]

for dep in pip_dependencies:
    print("Installing dependency " + dep)
    subprocess.check_call([pythonpath, '-m', 'pip', 'install'] + dep.split(' '))
