import os, sys, subprocess
from subprocess import CalledProcessError
import unreal

pip_dependencies = [
    "torch torchvision torchaudio --extra-index-url https://download.pytorch.org/whl/cu116",
    "diffusers",
    "transformers",
    "scipy",
    "ftfy"
]

def install_dependencies(pip_dependencies):
    pythonpath = unreal.Paths().make_standard_filename(os.path.join("..", "ThirdParty", "Python3", "Win64", "python.exe"))
    deps_installed = True
    for dep in pip_dependencies:
        print("Installing dependency " + dep)
        try:
            subprocess.check_call([pythonpath, '-m', 'pip', 'install', '--upgrade'] + dep.split(' '))
        except CalledProcessError as e:
            print("Return code for dependency {0} was non-zero. Returned {1} instead".format(dep, str(e.returncode)))
            deps_installed = False
    return deps_installed


SD_dependencies_installed = install_dependencies(pip_dependencies)
