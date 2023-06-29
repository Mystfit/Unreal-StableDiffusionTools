import os, sys, subprocess, shutil, io, time, select
import urllib.request
import importlib.util
from importlib.metadata import version
from importlib.metadata import PackageNotFoundError
from urllib.parse import urlparse
from pathlib import Path
from subprocess import CalledProcessError
from pkg_resources import parse_version
import unreal


# TODO: There's an unreal function to return this path
pythonpath = os.path.abspath(unreal.Paths().make_standard_filename(os.path.join("..", "ThirdParty", "Python3", "Win64", "python.exe")))
pythonheaders = os.path.abspath(unreal.Paths().make_standard_filename(os.path.join(unreal.Paths().engine_source_dir(), "ThirdParty", "Python3", "Win64", "include")))
pythonlibs = os.path.abspath(unreal.Paths().make_standard_filename(os.path.join(unreal.Paths().engine_source_dir(), "ThirdParty", "Python3", "Win64", "libs")))


@unreal.uclass()
class PyDependencyManager(unreal.DependencyManager):
    def __init__(self):
        unreal.DependencyManager.__init__(self)

    @unreal.ufunction(override=True)
    def install_dependency(self, dependency, force_reinstall):
        status = unreal.DependencyStatus()
        status.name = dependency.name
        status.status = unreal.DependencyState.UPDATING
        status.return_code = 0

        dep_name = f"{dependency.name}=={dependency.version}" if dependency.version else dependency.name
        dep_path = dependency.url if dependency.url else ""

        dep_force_upgrade = False
        extra_flags = dependency.args.split(' ') if dependency.args else []
        post_flags = ["--no-cache"] if dependency.no_cache else []

        if force_reinstall:
            extra_flags.append("--force-reinstall")
        print("Installing dependency " + dep_name)
        
        if dep_path:
            if dep_path.endswith(".whl"):
                print("Dowloading wheel")
                wheel_path = download_wheel(dep_name, dep_path)
                dep_name = f"{wheel_path}"
            elif ".git" in dep_path:
                print("Downloading git repository")
                dep_branch = f"@{dependency.branch}" if dependency.branch else ""
                dep_name = f"git+{dep_path}{dep_branch}#egg={dep_name}"
            else:
                dep_name = dep_path
        #else:
        #    dep_name = dep_name.split(' ')
            
        if dep_force_upgrade:
            extra_flags.append("--upgrade")

        try: 
            ext_site_packages = str(self.get_editor_property("PluginSitePackages"))
            environment = os.environ.copy()
            #existing_python_path = environment["PYTHONPATH"] if "PYTHONPATH" in environment else ""
            environment["PYTHONPATH"] = f"{ext_site_packages}"

            cmd = [f"{pythonpath}", '-u', '-m', 'pip', 'install', '--target', ext_site_packages, dep_name] + extra_flags + post_flags
            cmd_string = " ".join(cmd)
            print(f"Installing dependency using command '{cmd_string}'")
            proc = subprocess.Popen(
                cmd, 
                stdout=subprocess.PIPE, 
                stderr=subprocess.STDOUT,
                universal_newlines=True,
                shell=True,
                env=environment
            )
            for stdout_line in iter(proc.stdout.readline, ""):
                print(stdout_line)
                self.update_dependency_progress(dependency.name, str(stdout_line))
                # yield stdout_line
            proc.stdout.close()
            return_code = proc.wait()
            if return_code:
                err = proc.stderr.read() if proc.stderr else "No stderr pipe. Check normal log"
                self.update_dependency_progress(dependency.name, f"ERROR: Failed to install depdendency {dep_name}\nReturn code was {return_code}\nError was {err}")
                raise subprocess.CalledProcessError(return_code, " ".join(cmd))
            status.status = unreal.DependencyState.INSTALLED
            print(f"Return code for {dependency.name} was {return_code}")

        except CalledProcessError as e:
            status.status = unreal.DependencyState.ERROR
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
        module_name = dependency.module if dependency.module else dependency.name

        package_version = ""
        try:
            package_version = version(dependency.name)
        except PackageNotFoundError:
            pass
        status.version = package_version
        module_status = importlib.util.find_spec(module_name)
        status.status = unreal.DependencyState.INSTALLED if module_status else unreal.DependencyState.NOT_INSTALLED
        return status

    @unreal.ufunction(override=True)
    def all_dependencies_installed(self):
        dependencies = self.get_dependency_names()
        missing_deps = []
        for dependency in dependencies:
            status = self.get_dependency_status(dependency)
            if status.status != unreal.DependencyState.INSTALLED:
                missing_deps.append(dependency)
        if len(missing_deps):
            print(f"Missing dependencies: {missing_deps.join(', ')}")
            return False

        return True

    def clear_all_dependencies(self, env_dir, reset_system_deps):
        # Remove external site-packages dir
        if os.path.exists(env_dir):
            shutil.rmtree(env_dir)

        # Remove any leftover packages that are still in Unreal's base site-packages dir. 
        # Only really valid if we're upgrading our plugin from any version before 0.8.2
        if reset_system_deps:
            with open(Path(os.path.dirname(__file__)) / "requirements.txt") as f:
                requirements = [package.split('==')[0] for package in f.read().splitlines()]
                cmd = [f"{pythonpath}", '-m', 'pip', 'uninstall'] + requirements + ['-y']
                cmd_string = " ".join(cmd)
                print(f"Uninstalling dependencies using command '{cmd_string}'")
                try: 
                    proc = subprocess.Popen(
                        cmd, 
                        universal_newlines=True,
                        shell=True,
                    )
                except CalledProcessError as e:
                    print(f"pip returned status {e.returncode}")
                    if e.returncode:
                        print("pip log was:")
                        print(e.output)


def download_wheel(wheel_name, wheel_url):
    wheel_folder = os.path.normpath(os.path.join(unreal.Paths().engine_saved_dir(), "pythonwheels"))
    if not os.path.exists(wheel_folder):
        os.makedirs(wheel_folder)
    
    wheel_file = urlparse(wheel_url)
    wheel_path = os.path.normpath(os.path.join(wheel_folder, f"{os.path.basename(wheel_file.path)}"))
    if not os.path.exists(wheel_path):
        urllib.request.urlretrieve(wheel_url, wheel_path)
    
    return os.path.normpath(wheel_path)
