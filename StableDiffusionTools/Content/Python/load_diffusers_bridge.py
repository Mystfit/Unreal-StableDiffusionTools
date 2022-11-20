import unreal
from bridges import DiffusersBridge
import huggingface_hub
import torch
import transformers
import importlib

#importlib.reload(DiffusersBridge)
#importlib.reload(huggingface_hub)
#importlib.reload(torch)
#importlib.reload(transformers)
#importlib.reload(DiffusersBridge)

print("Creating default Diffusers bridge")
bridge = DiffusersBridge.DiffusersBridge()
subsystem = unreal.get_editor_subsystem(unreal.StableDiffusionSubsystem)
subsystem.set_editor_property("GeneratorBridge", bridge)
