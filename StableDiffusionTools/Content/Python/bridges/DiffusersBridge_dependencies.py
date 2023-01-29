import unreal

def GetDependencies():
    def MakeDependency(options):
        dependency_entry = unreal.DependencyManifestEntry()
        dependency_entry.set_editor_properties(options)
        return dependency_entry

    return [
        MakeDependency({"name":"torch", "args": "--extra-index-url https://download.pytorch.org/whl/cu117", "no_cache": False, "version": "1.13.1+cu117"}),
        MakeDependency({"name":"numpy", "version": "1.23.0"}),
        MakeDependency({"name":"transformers", "version": "4.26.0"}),
        MakeDependency({"name":"scipy", "version": "1.9.3"}),
        MakeDependency({"name":"ftfy", "version": "6.1.1"}),
        MakeDependency({"name":"accelerate", "version": "0.15.0"}),
        MakeDependency({"name":"xformers", "url":"https://github.com/Mystfit/xformers/releases/download/v0.15.0/xformers-0.0.15.dev0+5767ab4.d20221121-cp39-cp39-win_amd64.whl"}),
        MakeDependency({"name":"diffusers", "version": "0.12.1"}),
        MakeDependency({"name":"realesrgan", "version": "0.3.0"})
    ]
