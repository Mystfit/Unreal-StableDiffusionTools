import unreal

def GetDependencies():
    def MakeDependency(options):
        dependency_entry = unreal.DependencyManifestEntry()
        dependency_entry.set_editor_properties(options)
        return dependency_entry

    return [
        MakeDependency({"name":"torch", "args": "--extra-index-url https://download.pytorch.org/whl/cu117", "no_cache": False, "version": "1.13.1+cu117"}),
        MakeDependency({"name":"numpy", "version": "1.23.5"}),
        MakeDependency({"name":"safetensors", "version": "0.3.0"}),
        MakeDependency({"name":"transformers", "version": "4.26.1"}),
        MakeDependency({"name":"scipy", "version": "1.10.0"}),
        MakeDependency({"name":"ftfy", "version": "6.1.1"}),
        MakeDependency({"name":"accelerate", "version": "0.17.1"}),
        MakeDependency({"name":"xformers", "version":"0.0.16"}),
        MakeDependency({"name":"diffusers", "url": "https://github.com/huggingface/diffusers.git"}),
        MakeDependency({"name":"controlnet_hinter", "url": "https://github.com/takuma104/controlnet_hinter.git"}),
        MakeDependency({"name":"realesrgan", "version": "0.3.0"})
    ]
