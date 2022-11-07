import importlib.util
import unreal
import signal
import install_dependencies

# Replace print() command to fix Unreal flagging every Python print call as an error 
print = unreal.log

# Redirect missing SIGKILL signal on windows to SIGTERM
signal.SIGKILL = signal.SIGTERM


def SD_dependencies_installed():
    dependencies = install_dependencies.dependencies
    installed = True
    modules = [package_opts["module"] if "module" in package_opts else package_name for package_name, package_opts in dependencies.items()]
    for module in modules:
        print(f"Looking for module {module}")
        try:
            if not importlib.util.find_spec(module):
                raise(ValueError())
        except ValueError:
            print("Missing Stable Diffusion dependency {0}. Please install or update the plugin's python dependencies".format(module))
            installed = False

    return installed


# Check if dependencies are installed correctly
dependencies_installed = SD_dependencies_installed()
print("Stable Diffusion dependencies are {0}available".format("" if dependencies_installed else "not "))

if SD_dependencies_installed:
    try:
        import load_diffusers_bridge
    except ImportError:
        print("Skipping default Diffusers Bridge load until dependencies have been installed")

