import unreal

def GetDependencies():
    def MakeDependency(options):
        dependency_entry = unreal.DependencyManifestEntry()
        dependency_entry.set_editor_properties(options)
        return dependency_entry

    torch_index_url = "--extra-index-url https://download.pytorch.org/whl/cu117"
    return [
        MakeDependency({"name":"torch", "args": torch_index_url}),
        MakeDependency({"name":"torchvision", "args": torch_index_url}),
        MakeDependency({"name":"torchaudio", "args": torch_index_url}),
        MakeDependency({"name":"diffusers", "url": "https://github.com/huggingface/diffusers.git@326de4191578dfb55cb968880d40d703075e331e"}),
        MakeDependency({"name":"transformers"}),
        MakeDependency({"name":"numpy"}),
        MakeDependency({"name":"scipy"}),
        MakeDependency({"name":"ftfy"}),
        MakeDependency({"name":"realesrgan"}),
        MakeDependency({"name":"accelerate"}),
        MakeDependency({"name":"xformers", "url":"https://github.com/Mystfit/xformers/releases/download/v0.15.0/xformers-0.0.15.dev0+5767ab4.d20221121-cp39-cp39-win_amd64.whl"}),
    ]
