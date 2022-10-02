print("Inside StableDiffusionTools init_unreal.py")
import DiffusersBridge

bridge = DiffusersBridge.DiffusersBridge()
subsystem = unreal.get_editor_subsystem(unreal.StableDiffusionSubsystem)
subsystem.set_editor_property("GeneratorBridge", bridge)
