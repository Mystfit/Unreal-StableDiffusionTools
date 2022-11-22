import unreal
import importlib, pathlib, os, signal
import install_dependencies


# Get all python files in a folder
def get_py_files(src):
    #cwd = os.getcwd() # Current Working directory
    py_files = [] 
    for root, dirs, files in os.walk(src):
        for file in files:
            if file.endswith(".py"):
                py_files.append(os.path.join(root, file))
                print(os.path.join(root, file))
    return py_files

# Dynamically import a list of python files
def dynamic_import(module_name, py_path):
    module_spec = importlib.util.spec_from_file_location(module_name, py_path)
    module = importlib.util.module_from_spec(module_spec)
    module_spec.loader.exec_module(module)
    return module

# Recursively load all python files in a directory
def dynamic_import_from_src(src, star_import = False):
    my_py_files = get_py_files(src)
    for py_file in my_py_files:
        module_name = os.path.split(py_file)[-1].strip(".py")
        imported_module = dynamic_import(module_name, py_file)
        if star_import:
            for obj in dir(imported_module):
                globals()[obj] = imported_module.__dict__[obj]
        else:
            globals()[module_name] = imported_module
    return


# Replace print() command to fix Unreal flagging every Python print call as an error (doesn't work)
print = unreal.log

# Redirect missing SIGKILL signal on windows to SIGTERM
signal.SIGKILL = signal.SIGTERM

# Load dependency manager
dependency_manager = install_dependencies.PyDependencyManager()
subsystem = unreal.get_editor_subsystem(unreal.StableDiffusionSubsystem)
subsystem.set_editor_property("DependencyManager", dependency_manager)

# Import all bridges so we can pick which derived class we want to use in the plugin editor settings
dynamic_import_from_src(os.path.join(pathlib.Path(__file__).parent.resolve(), "bridges"))

# Let Unreal know we've finished loading our init script
subsystem.set_editor_property("PythonLoaded", True)
subsystem.on_python_loaded_ex.broadcast()
