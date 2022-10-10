import unreal
import DiffusersBridge
import huggingface_hub

import importlib
importlib.reload(DiffusersBridge)
importlib.reload(huggingface_hub)

print("Creating default Diffusers bridge")
bridge = DiffusersBridge.DiffusersBridge()
subsystem = unreal.get_editor_subsystem(unreal.StableDiffusionSubsystem)
subsystem.set_editor_property("GeneratorBridge", bridge)
