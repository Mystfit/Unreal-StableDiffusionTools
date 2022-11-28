import os, sys, subprocess
import urllib.request
import importlib.util

from urllib.parse import urlparse

from subprocess import CalledProcessError
import unreal

pythonpath = unreal.get_interpreter_executable_path()
#pythonheaders = os.path.abspath(unreal.Paths().make_standard_filename(os.path.join(unreal.Paths().engine_source_dir(), "ThirdParty", "Python3", "Win64", "include")))
#pythonlibs = os.path.abspath(unreal.Paths().make_standard_filename(os.path.join(unreal.Paths().engine_source_dir(), "ThirdParty", "Python3", "Win64", "libs")))


@unreal.uclass()
class PyDependencyManager(unreal.DependencyManager):
    def __init__(self):
        unreal.DependencyManager.__init__(self)

    #@unreal.ufunction(override=True)
    #def install_all_dependencies(self, force_reinstall):
    #    for dependency in dependencies.keys():
    #        install_dependency(dependency)

    @unreal.ufunction(override=True)
    def install_dependency(self, dependency, force_reinstall):
        status = unreal.DependencyStatus()
        status.name = dependency.name
        status.installed = False
        status.return_code = 0

        dep_name = dependency.name
        dep_path = dependency.url if dependency.url else ""
        dep_force_upgrade = True
        extra_flags = dependency.args.split(' ') if dependency.args else []
        if force_reinstall:
            extra_flags.append("--force-reinstall")
        print("Installing dependency " + dep_name)
        
        if dep_path:
            if dep_path.endswith(".whl"):
                print("Dowloading wheel")
                wheel_path = download_wheel(dep_name, dep_path)
                dep_name = [f"{wheel_path}"]
            elif dep_path.endswith(".git"):
                print("Downloading git repository")
                #path = clone_dependency(dep_name, dep_path)
                dep_name = [f"git+{dep_path}#egg={dep_name}"]
                #extra_flags += ["--global-option=build_ext", f"--global-option=-I'{pythonheaders}'", f"--global-option=-L'{pythonlibs}'"]
        else:
            dep_name = dep_name.split(' ')
            
        if dep_force_upgrade:
            extra_flags.append("--upgrade")

        try: 
            cmd = [pythonpath, '-m', 'pip', 'install'] + extra_flags + dep_name
            proc = subprocess.Popen(
                cmd, 
                stdout=subprocess.PIPE, 
                stderr=subprocess.PIPE,
                universal_newlines=True,
                shell=True
            )
            for stdout_line in iter(proc.stdout.readline, ""):
                print(stdout_line)
                self.update_dependency_progress(dependency.name, str(stdout_line))
                # yield stdout_line
            proc.stdout.close()
            return_code = proc.wait()
            if return_code:
                err = proc.stderr.read()
                self.update_dependency_progress(dependency.name, f"ERROR: Failed to install depdendency {dep_name}\nReturn code was {return_code}\nError was {err}")
                raise subprocess.CalledProcessError(return_code, " ".join(cmd))
            status.installed = True
        except CalledProcessError as e:
            status.installed = False
            status.message = e.output if e.output else ""
            status.return_code = e.returncode

        return status

    @unreal.ufunction(override=True)
    def get_dependency_names(self):
        return []
        #return [package_name for package_name in dependencies.keys()]

    @unreal.ufunction(override=True)
    def get_dependency_status(self, dependency):
        status = unreal.DependencyStatus()
        status.name = dependency.name
        status.version = "None"
        module_status = importlib.util.find_spec(dependency.module)
        status.installed = True if module_status else False
        print(f"Module {dependency.module} installed for dependency {dependency.name}: {status.installed}")
        return status

    @unreal.ufunction(override=True)
    def all_dependencies_installed(self):
        dependencies = self.get_dependency_names()
        dependencies_installed = True
        for dependency in dependencies:
            status = self.get_dependency_status(dependency)
            if not status.installed:
                dependencies_installed = False
        print(f"All dependencies installed? {dependencies_installed}")
        return dependencies_installed


def clone_dependency(repo_name, repo_url):
    from git import Repo, exc
    import git

    print("Cloning dependency " + repo_name)
    repo_path = os.path.join(unreal.Paths().engine_saved_dir(), "pythonrepos", repo_name)
    repo = None
    try:
        repo = Repo.clone_from(repo_url, repo_path)
    except git.exc.GitCommandError as e:
        print(f"Could not clone repo from {repo_url}. Reason was {e}")
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

    print(f"No local git repo found at {repo_path}")
    return None


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
    SD_dependencies_installed = True
    for dep_name in dependencies.keys():
        if not install_dependencies(dep_name):
            SD_dependencies_installed = False
