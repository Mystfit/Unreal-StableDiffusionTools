import unreal

def GetDependencies():
    def MakeDependency(options):
        dependency_entry = unreal.DependencyManifestEntry()
        dependency_entry.set_editor_properties(options)
        return dependency_entry

    return [
        MakeDependency({"name":"torch", "no_cache": True, "args": "--index-url https://download.pytorch.org/whl/cu118", "version": "2.0.0+cu118"}), #"2.0.0+cu118"
        MakeDependency({"name":"torchvision", "no_cache": True, "args": "--index-url https://download.pytorch.org/whl/cu118", "version": "0.15.1"}),
        MakeDependency({"name":"numpy", "version": "1.24.2"}), #"1.23.5"
        MakeDependency({"name":"safetensors", "version": "0.3.0"}),
        MakeDependency({"name":"transformers", "version": "4.27.3"}),
        MakeDependency({"name":"scipy", "version": "1.10.1"}),
        MakeDependency({"name":"ftfy", "version": "6.1.1"}),
        MakeDependency({"name":"accelerate", "version": "0.17.1"}),
        MakeDependency({"name":"diffusers", "url": "https://github.com/mikegarts/diffusers@img2img.multiple.controlnets.pipeline"}),
        MakeDependency({"name":"compel", "version":"1.0.4"}),
        MakeDependency({"name":"opencv-python", "version":"4.7.0.72"})
    ]
