import os, sys, subprocess
import urllib.request
from urllib.parse import urlparse

from subprocess import CalledProcessError
import unreal

dependencies = {
    "gitpython": {"module": "git"},
    "torch": {"args": "--extra-index-url https://download.pytorch.org/whl/cu117"},
    "torchvision":  {"args": "--extra-index-url https://download.pytorch.org/whl/cu117"},
    "torchaudio":  {"args": "--extra-index-url https://download.pytorch.org/whl/cu117"},
    "diffusers": {},
    "transformers": {},
    "scipy":{},
    "ftfy": {},
    "realesrgan": {},
    "accelerate": {},
    "xformers": {"url": "https://github.com/Mystfit/xformers/releases/download/v0.0.14/xformers-0.0.14.dev0-cp39-cp39-win_amd64.whl", "upgrade": True}
}

# TODO: There's an unreal function to return this path
pythonpath = os.path.abspath(unreal.Paths().make_standard_filename(os.path.join("..", "ThirdParty", "Python3", "Win64", "python.exe")))
pythonheaders = os.path.abspath(unreal.Paths().make_standard_filename(os.path.join(unreal.Paths().engine_source_dir(), "ThirdParty", "Python3", "Win64", "include")))
pythonlibs = os.path.abspath(unreal.Paths().make_standard_filename(os.path.join(unreal.Paths().engine_source_dir(), "ThirdParty", "Python3", "Win64", "libs")))


def install_dependencies(pip_dependencies):
    deps_installed = True

    with unreal.ScopedSlowTask(len(pip_dependencies), "Installing dependencies") as slow_task:
        slow_task.make_dialog(True)
        for dep_name, dep_options in pip_dependencies.items():
            dep_path = dep_options["url"] if "url" in dep_options.keys() else ""
            dep_force_upgrade = dep_options["upgrade"] if "upgrade" in dep_options.keys() else True
            extra_flags = dep_options["args"].split(' ') if "args" in dep_options.keys() else []
            print("Installing dependency " + dep_name)
            if slow_task.should_cancel():         # True if the user has pressed Cancel in the UI
                break
            
            slow_task.enter_progress_frame(1.0, f"Installing dependency {dep_name}")
            if dep_path:
                if dep_path.endswith(".whl"):
                    wheel_path = download_wheel(dep_name, dep_path)
                    dep_name = [f"{wheel_path}"]
                elif dep_path.endswith(".git"):
                    path = clone_dependency(dep_name, dep_path)
                    dep_name = [f"{path}"]
                    extra_flags += ["--global-option=build_ext", f"--global-option=-I'{pythonheaders}'", f"--global-option=-L'{pythonlibs}'"]
            else:
                dep_name = dep_name.split(' ')
               
            if dep_force_upgrade:
                extra_flags.append("--upgrade")

            try: 
                print(subprocess.check_output([f"{pythonpath}", '-m', 'pip', 'install'] + extra_flags + dep_name), stdout=subprocess.PIPE, stderr=subprocess.PIPE)
            except CalledProcessError as e:
                print("Return code for dependency {0} was non-zero. Returned {1} instead".format(dep_name, str(e.returncode)))
                print("Command:")
                print(" ".join([f"{pythonpath}", '-m', 'pip', 'install'] + extra_flags + dep_name))
                print("Output:")
                print(e.output)
                deps_installed = False
    return deps_installed


def clone_dependency(repo_name, repo_url):
    from git import Repo, exc
    import git

    print("Cloning dependency " + repo_name)
    repo_path = os.path.join(unreal.Paths().engine_saved_dir(), "pythonrepos", repo_name)
    repo = None
    try:
        repo = Repo.clone_from(repo_url, repo_path)
    except git.exc.GitCommandError as e:
        repo = Repo(repo_path)

    if repo:
        # Handle long path lengths for windows
        # Make sure long paths are enabled at the system level
        # https://learn.microsoft.com/en-us/answers/questions/730467/long-paths-not-working-in-windows-2019.html
        repo.config_writer().set_value("core", "longpaths", True).release()

        # Make sure the repo has all submodules available
        output = repo.git.submodule('update', '--init', '--recursive')
        requirements_file = os.path.normpath(os.path.join(repo_path, "requirements.txt"))

        # Update repo dependencies
        if os.path.exists(requirements_file):
            try:
                subprocess.check_call([pythonpath, '-m', 'pip', 'install', '-r', requirements_file])
            except CalledProcessError as e:
                print("Failed to install repo requirements.txt")
                command = " ".join([pythonpath, '-m', 'pip', 'install', '-r', requirements_file])
                print(f"Command: {command}")

        return os.path.normpath(repo_path)


def download_wheel(wheel_name, wheel_url):
    wheel_folder = os.path.normpath(os.path.join(unreal.Paths().engine_saved_dir(), "pythonwheels"))
    if not os.path.exists(wheel_folder):
        os.makedirs(wheel_folder)
    
    wheel_file = urlparse(wheel_url)
    wheel_path = os.path.normpath(os.path.join(wheel_folder, f"{os.path.basename(wheel_file.path)}"))
    if not os.path.exists(wheel_path):
        urllib.request.urlretrieve(wheel_url, wheel_path)
    
    return os.path.normpath(wheel_path)


if __name__ == "__main__":
    global SD_dependencies_installed
    SD_dependencies_installed = install_dependencies(dependencies)
