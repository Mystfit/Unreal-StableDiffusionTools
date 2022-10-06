SD_dependencies_installed = False

try:
    import diffusers
    import PIL
    import torch
    SD_dependencies_installed = True
    print("Stable Diffusion dependencies are available")
except ImportError as e:
    print("Stable Diffusion plugin python dependencies not installed")

try:
    import DiffusersBridge
    print("Creating default Diffusers bridge")
    subsystem = unreal.get_editor_subsystem(unreal.StableDiffusionSubsystem)
    subsystem.set_editor_property("GeneratorBridge", DiffusersBridge.DiffusersBridge())
except ImportError:
    print("Skipping default Diffusers Bridge load until dependencies have been installed")
