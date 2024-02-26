import unreal

def GetDependencies():
    def MakeDependency(options):
        dependency_entry = unreal.DependencyManifestEntry()
        dependency_entry.set_editor_properties(options)
        return dependency_entry

    return [
        MakeDependency({"name":"torch", "no_cache": False, "version": "2.2.1+cu121", "args": "--index-url https://download.pytorch.org/whl/cu121"}),
        MakeDependency({"name":"torchvision", "no_cache": False, "version": "0.17.1+cu118", "args": "--index-url https://download.pytorch.org/whl/cu121"}),
        MakeDependency({"name":"numpy", "version": "1.24.4"}), 
        MakeDependency({"name":"safetensors", "version": "0.4.2"}),
        MakeDependency({"name":"transformers", "version": "4.38.1"}),
        MakeDependency({"name":"scipy", "version": "1.12.0"}),
        MakeDependency({"name":"ftfy", "version": "6.1.3"}),
        MakeDependency({"name":"accelerate", "version": "0.27.2"}),
        MakeDependency({"name":"diffusers", "version": "0.26.3"}),
        MakeDependency({"name":"compel", "version": "2.0.2"}),
        MakeDependency({"name":"opencv-python", "version":"4.9.0.80", "module": "cv2"}),
        MakeDependency({"name":"realesrgan", "version": "0.3.0"}),
        MakeDependency({"name":"tqdm", "version": "4.66.2"}),
        MakeDependency({"name":"pywavelets", "version": "1.5.0"}),
        MakeDependency({"name":"invisible-watermark", "version": "0.2.0"}),
        MakeDependency({"name":"omegaconf", "version": "2.3.0"})
    ]
