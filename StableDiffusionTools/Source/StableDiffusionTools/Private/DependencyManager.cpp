#include "DependencyManager.h"
#include "Interfaces/IPluginManager.h"
#include "Async/ASync.h"
#include "IPythonScriptPlugin.h"
#include "DependencySettings.h"
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

void UDependencyManager::SetIsInstallingDependencies(bool State)
{
	bIsInstallingDependencies = State;
}

bool UDependencyManager::IsInstallingDependencies() const
{
	return bIsInstallingDependencies;
}

void UDependencyManager::RestartAndUpdateDependencies()
{
	auto Settings = GetMutableDefault<UDependencySettings>();
	Settings->AutoLoadBridgeScripts = false;
	Settings->AutoUpdateDependenciesOnStartup = true;
	Settings->SaveConfig();
	UStableDiffusionBlueprintLibrary::RestartEditor();
}

void UDependencyManager::ResetDependencies(bool ClearSystemDeps)
{
	auto Settings = GetMutableDefault<UDependencySettings>();
	Settings->ClearDependenciesOnEditorRestart = true;
	Settings->ClearSystemDependenciesOnEditorRestart = ClearSystemDeps;
	Settings->SaveConfig();
	//RestartAndUpdateDependencies();
}

void UDependencyManager::FinishedClearingDependencies()
{
	auto Settings = GetMutableDefault<UDependencySettings>();
	Settings->ClearDependenciesOnEditorRestart = false;
	Settings->ClearSystemDependenciesOnEditorRestart = false;
	Settings->SaveConfig();
}

void UDependencyManager::FinishedUpdatingDependencies()
{
	auto Settings = GetMutableDefault<UDependencySettings>();
	Settings->AutoLoadBridgeScripts = true;
	Settings->ClearDependenciesOnEditorRestart = false;
	Settings->ClearSystemDependenciesOnEditorRestart = false;
	Settings->AutoUpdateDependenciesOnStartup = false;
	Settings->SaveConfig();
	UStableDiffusionBlueprintLibrary::RestartEditor();
}

void UDependencyManager::UpdateDependencyProgress(FString Dependency, FString Line)
{
	AsyncTask(ENamedThreads::GameThread, [this, Dependency, Line]() {
		OnDependencyProgress.Broadcast(Dependency, Line);
	});
}

FString UDependencyManager::GetPluginVersionName() {
	const TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin(TEXT("StableDiffusionTools"));
	if (Plugin.IsValid())
	{
		return Plugin->GetDescriptor().VersionName;
	}

	return "0.0.0";
}
