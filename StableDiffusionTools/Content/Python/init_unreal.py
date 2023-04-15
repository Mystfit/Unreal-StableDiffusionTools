import unreal
import importlib, pathlib, os, signal, venv, sys
import dependency_manager


# Get all python files in a folder
def get_py_files(src, ends_with):
    #cwd = os.getcwd() # Current Working directory
    py_files = [] 
    for root, dirs, files in os.walk(src):
        for file in files:
            if file.endswith(ends_with):
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
def dynamic_import_from_src(src, ends_with, star_import = False):
    my_py_files = get_py_files(src, ends_with)
    for py_file in my_py_files:
        module_name = os.path.split(py_file)[-1].strip(".py")
        imported_module = dynamic_import(module_name, py_file)
        if star_import:
            for obj in dir(imported_module):
                globals()[obj] = imported_module.__dict__[obj]
        else:
            globals()[module_name] = imported_module
    return

# Load all manifest files that should be located next to each bridge file
def load_manifests(manifest_dir):
    # Get manifest objects
    manifests = {}
    manifest_files = get_py_files(manifest_dir, "_dependencies.py")
    for manifest in manifest_files:
        manifest_module_name = os.path.split(manifest)[-1][:-len("_dependencies.py")]
        imported_module = dynamic_import(manifest_module_name, manifest)

        print(manifest_module_name)
        manifest = unreal.DependencyManifest()
        manifest_entries = imported_module.GetDependencies()
        manifest.set_editor_property("ManifestEntries", manifest_entries)
        manifests[manifest_module_name] = manifest

    return manifests


# Replace print() command to fix Unreal flagging every Python print call as an error (doesn't work)
print = unreal.log

# Redirect missing SIGKILL signal on windows to SIGTERM
signal.SIGKILL = signal.SIGTERM

# Get plugin startup options
dependency_options = unreal.StableDiffusionBlueprintLibrary.get_dependency_options()

# Set up virtual environment
env_dir = pathlib.Path(unreal.Paths().engine_saved_dir()) / "StableDiffusionToolsPyEnv"
env_site_packages = env_dir / "Lib" / "site-packages"
print(f"Dependency installation dir: {env_site_packages}")

# Setup a new virtual environment to contain our downloaded python dependencies
if not os.path.exists(env_site_packages):
    os.makedirs(env_site_packages)
sys.path.append(str(env_site_packages))

# Load dependency manager
dep_manager = dependency_manager.PyDependencyManager()
dep_manager.set_editor_property("PluginSitePackages", str(env_site_packages))
subsystem = unreal.get_editor_subsystem(unreal.StableDiffusionSubsystem)
subsystem.set_editor_property("DependencyManager", dep_manager)

# Nuke dependencies before loading them if we're trying to reset the editor dependencies
reset_deps = dependency_options.get_editor_property("ClearDependenciesOnEditorRestart")
if reset_deps:
    print(f"Clearing python dependendencies")
    dep_manager.clear_all_dependencies(env_dir)

    # Flag dependencies as cleared so we don't keep clearing them every restart
    dep_manager.finished_clearing_dependencies()

# Location of plugin bridge files
bridge_dir = os.path.join(pathlib.Path(__file__).parent.resolve(), "bridges")

# Loads a map of all bridge names and dependencies that need to be installed to import/run the bridge
dep_manager.set_editor_property("DependencyManifests", load_manifests(bridge_dir))

# Import all bridges so we can pick which derived class we want to use in the plugin editor settings
if dependency_options.get_editor_property("AutoLoadBridgeScripts"):
    dynamic_import_from_src(os.path.join(pathlib.Path(__file__).parent.resolve(), "bridges"), "Bridge.py")

# Let Unreal know we've finished loading our init script
subsystem.set_editor_property("PythonLoaded", True)
subsystem.on_python_loaded_ex.broadcast()
