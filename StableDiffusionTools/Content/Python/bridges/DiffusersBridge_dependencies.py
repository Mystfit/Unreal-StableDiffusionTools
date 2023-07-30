import unreal

def GetDependencies():
    def MakeDependency(options):
        dependency_entry = unreal.DependencyManifestEntry()
        dependency_entry.set_editor_properties(options)
        return dependency_entry

    return [
        MakeDependency({"name":"torch", "no_cache": False, "version": "2.0.1+cu118", "args": "--index-url https://download.pytorch.org/whl/cu118"}), #"2.0.0+cu118"
        MakeDependency({"name":"torchvision", "no_cache": False, "version": "0.15.2+cu118", "args": "--index-url https://download.pytorch.org/whl/cu118"}),
        MakeDependency({"name":"numpy", "version": "1.24.3"}), 
        MakeDependency({"name":"safetensors", "version": "0.3.1"}),
        MakeDependency({"name":"transformers", "version": "4.29.2"}),
        MakeDependency({"name":"scipy", "version": "1.10.1"}),
        MakeDependency({"name":"ftfy", "version": "6.1.1"}),
        MakeDependency({"name":"accelerate", "version": "0.19.0"}),
        MakeDependency({"name":"diffusers", "url": "https://github.com/Mystfit/diffusers.git", "branch":"unreal_fixes"}),
        MakeDependency({"name":"compel", "version": "2.0.0"}),
        MakeDependency({"name":"opencv-python", "version":"4.7.0.72", "module": "cv2"}),
        MakeDependency({"name":"realesrgan", "version": "0.3.0"}),
        MakeDependency({"name":"tqdm", "version": "4.65.0"}),
        MakeDependency({"name":"pywavelets", "version": "1.4.1"}),
        MakeDependency({"name":"invisible-watermark", "version": "0.2.0"}),
        MakeDependency({"name":"omegaconf", "version": "2.3.0"})
    ]
