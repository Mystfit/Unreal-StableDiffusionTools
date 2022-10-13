import importlib.util
import unreal

# Replace print() command to fix Unreal flagging every Python print call as an error 
print = unreal.log

modules = [
    "diffusers",
    "PIL",
    "torch",
    "huggingface_hub",
    "transformers"
]
SD_dependencies_installed = True
for module in modules:
    try:
        importlib.util.find_spec(module)
    except ValueError:
        print("Missing Stable Diffusion dependency {0}. Please install or update the plugin's python dependencies".format(module))
        SD_dependencies_installed = False
     
    
print("Stable Diffusion dependencies are {0}available".format("" if SD_dependencies_installed else "not "))

if SD_dependencies_installed:
    try:
        import load_diffusers_bridge
    except ImportError:
        print("Skipping default Diffusers Bridge load until dependencies have been installed")
