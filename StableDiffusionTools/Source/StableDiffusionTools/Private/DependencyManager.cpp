#include "DependencyManager.h"
#include "Async/ASync.h"
#include "IPythonScriptPlugin.h"
#include "StableDiffusionToolsSettings.h"
#include "StableDiffusionBlueprintLibrary.h"


UDependencyManager::UDependencyManager(const FObjectInitializer& initializer)
{
	// Wait for Python to load our derived classes before we construct the bridge
	IPythonScriptPlugin& PythonModule = FModuleManager::LoadModuleChecked<IPythonScriptPlugin>(TEXT("PythonScriptPlugin"));
	PythonModule.OnPythonInitialized().AddLambda([this]() {
		OnPythonReady.Broadcast();
	});
}

void UDependencyManager::ResetDependencies()
{
	auto Settings = GetMutableDefault<UStableDiffusionToolsSettings>();
	Settings->ClearDependenciesOnEditorRestart = true;
	Settings->SaveConfig();
	UStableDiffusionBlueprintLibrary::RestartEditor();
}

void UDependencyManager::FinishedClearingDependencies()
{
	auto Settings = GetMutableDefault<UStableDiffusionToolsSettings>();
	Settings->ClearDependenciesOnEditorRestart = false;
	Settings->SaveConfig();
}

void UDependencyManager::UpdateDependencyProgress(FString Dependency, FString Line)
{
	AsyncTask(ENamedThreads::GameThread, [this, Dependency, Line]() {
		OnDependencyProgress.Broadcast(Dependency, Line);
	});
}
