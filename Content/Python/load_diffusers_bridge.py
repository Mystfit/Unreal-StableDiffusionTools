import unreal
import DiffusersBridge

print("Creating default Diffusers bridge")
bridge = DiffusersBridge.DiffusersBridge()
subsystem = unreal.get_editor_subsystem(unreal.StableDiffusionSubsystem)
subsystem.set_editor_property("GeneratorBridge", bridge)
