import unreal
import signal
import install_dependencies

# Replace print() command to fix Unreal flagging every Python print call as an error 
print = unreal.log

# Redirect missing SIGKILL signal on windows to SIGTERM
signal.SIGKILL = signal.SIGTERM

# Load dependency manager
dependency_manager = install_dependencies.PyDependencyManager()
subsystem = unreal.get_editor_subsystem(unreal.StableDiffusionSubsystem)
subsystem.set_editor_property("DependencyManager", dependency_manager)

if dependency_manager.all_dependencies_installed():
    try:
        import load_diffusers_bridge
    except ImportError:
        print("Skipping default Diffusers Bridge load until dependencies have been installed")
