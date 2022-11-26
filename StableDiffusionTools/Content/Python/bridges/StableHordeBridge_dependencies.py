import unreal

def GetDependencies():
    def MakeDependency(options):
        dependency_entry = unreal.DependencyManifestEntry()
        dependency_entry.set_editor_properties(options)
        return dependency_entry

    return [
        MakeDependency({"name":"requests"})
    ]
