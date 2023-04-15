import unreal

def GetDependencies():
    def MakeDependency(options):
        dependency_entry = unreal.DependencyManifestEntry()
        dependency_entry.set_editor_properties(options)
        return dependency_entry

    return [
        MakeDependency({"name":"torch", "no_cache": False, "version": "2.0.0+cu118", "args": "--index-url https://download.pytorch.org/whl/cu118"}), #"2.0.0+cu118"
        MakeDependency({"name":"torchvision", "no_cache": False, "version": "0.15.1+cu118", "args": "--index-url https://download.pytorch.org/whl/cu118"}),
        MakeDependency({"name":"numpy", "version": "1.24.2"}), 
        MakeDependency({"name":"safetensors", "version": "0.3.0"}),
        MakeDependency({"name":"transformers", "version": "4.27.3"}),
        MakeDependency({"name":"scipy", "version": "1.10.1"}),
        MakeDependency({"name":"ftfy", "version": "6.1.1"}),
        MakeDependency({"name":"accelerate", "version": "0.18.0"}),
        MakeDependency({"name":"diffusers", "version": "0.15.0"}),
        MakeDependency({"name":"compel", "version": "1.1.3"}),
        MakeDependency({"name":"opencv-python", "version":"4.7.0.72"})
    ]
