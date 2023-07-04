#include "StableDiffusionToolsModule.h"
#include "ISettingsModule.h"
#include "ISettingsSection.h"
#include "StableDiffusionToolsSettings.h"
#include "StableDiffusionSubsystem.h"
#include "AssetToolsModule.h"
#include "IPythonScriptPlugin.h"

#define LOCTEXT_NAMESPACE "FStableDiffusionToolsModule"

void FStableDiffusionToolsModule::StartupModule() 
{
	// Wait for Python to load our derived classes before we construct the settings object
	IPythonScriptPlugin& PythonModule = FModuleManager::LoadModuleChecked<IPythonScriptPlugin>(TEXT("PythonScriptPlugin"));
	PythonModule.OnPythonInitialized().AddRaw(this, &FStableDiffusionToolsModule::CreateSettingsSection);
}

void FStableDiffusionToolsModule::ShutdownModule() {

}

void FStableDiffusionToolsModule::CreateSettingsSection() {
	ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings");
	if (SettingsModule != nullptr)
	{
		ISettingsSectionPtr SettingsSection = SettingsModule->RegisterSettings("Project", "Plugins", "StableDiffusionTools",
			LOCTEXT("StableDiffusionToolsSettingsName", "Stable Diffusion Tools"),
			LOCTEXT("StableDiffusionToolsSettingsDescription", "Configure the Stable Diffusion tools plug-in."),
			GetMutableDefault<UStableDiffusionToolsSettings>()
		);

		if (SettingsSection.IsValid())
		{
			// Run gets to populate defaults
			GetMutableDefault<UStableDiffusionToolsSettings>()->GetPythonSitePackagesOverridePath();
			GetMutableDefault<UStableDiffusionToolsSettings>()->GetModelDownloadPath();

			SettingsSection->OnModified().BindRaw(this, &FStableDiffusionToolsModule::HandleSettingsSaved);
		}

		// Delayed property registration so that we can see all of our python bridges
		FPropertyEditorModule& PropertyModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
		PropertyModule.RegisterCustomClassLayout(UStableDiffusionToolsSettings::StaticClass()->GetFName(), FOnGetDetailCustomizationInstance::CreateStatic(&FStableDiffusionToolsSettingsDetails::MakeInstance));
		PropertyModule.NotifyCustomizationModuleChanged();
	}
}

void FStableDiffusionToolsModule::CreateDetailCustomizations()
{

}


bool FStableDiffusionToolsModule::HandleSettingsSaved() 
{
	UStableDiffusionToolsSettings* Settings = GetMutableDefault<UStableDiffusionToolsSettings>();
	if (Settings->GetGeneratorType()) {

		Settings->AddGeneratorToken(Settings->GetGeneratorType()->GetFName());
		if (Settings->GetGeneratorType() != UStableDiffusionBridge::StaticClass()) {
			Settings->SaveConfig();
		}
	}

	UStableDiffusionSubsystem* SDSubSystem = GEditor->GetEditorSubsystem<UStableDiffusionSubsystem>();
	if (SDSubSystem) {
		SDSubSystem->CreateBridge(Settings->GetGeneratorType());
		return true;
	}
	return false;
}

IMPLEMENT_MODULE(FStableDiffusionToolsModule, StableDiffusionTools)

#undef LOCTEXT_NAMESPACE
