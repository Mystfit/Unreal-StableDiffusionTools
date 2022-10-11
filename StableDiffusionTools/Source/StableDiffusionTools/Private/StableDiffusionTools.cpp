// Copyright Epic Games, Inc. All Rights Reserved.

#include "StableDiffusionTools.h"
#include "StableDiffusionToolsStyle.h"
#include "StableDiffusionToolsCommands.h"
#include "StableDiffusionSubsystem.h"

#include "LevelEditor.h"
#include "Widgets/Docking/SDockTab.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Text/STextBlock.h"
#include "ToolMenus.h"
#include "Engine/ObjectLibrary.h"
#include "AssetRegistryModule.h"
#include "EditorUtilityWidget.h"

static const FName StableDiffusionToolsTabName("StableDiffusionTools");

#define LOCTEXT_NAMESPACE "FStableDiffusionToolsModule"

void FStableDiffusionToolsModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
	
	FStableDiffusionToolsStyle::Initialize();
	FStableDiffusionToolsStyle::ReloadTextures();
	FStableDiffusionToolsCommands::Register();
		
	PluginCommands = MakeShareable(new FUICommandList);
	PluginCommands->MapAction(
		FStableDiffusionToolsCommands::Get().OpenPluginWindow,
		FExecuteAction::CreateRaw(this, &FStableDiffusionToolsModule::PluginButtonClicked),
		FCanExecuteAction());

	UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FStableDiffusionToolsModule::RegisterMenus));
	
	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(StableDiffusionToolsTabName, FOnSpawnTab::CreateRaw(this, &FStableDiffusionToolsModule::OnSpawnPluginTab))
		.SetDisplayName(LOCTEXT("FStableDiffusionToolsTabTitle", "StableDiffusionTools"))
		.SetMenuType(ETabSpawnerMenuType::Hidden);
}

void FStableDiffusionToolsModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.

	UToolMenus::UnRegisterStartupCallback(this);

	UToolMenus::UnregisterOwner(this);

	FStableDiffusionToolsStyle::Shutdown();

	FStableDiffusionToolsCommands::Unregister();

	FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(StableDiffusionToolsTabName);
}

TSharedRef<SDockTab> FStableDiffusionToolsModule::OnSpawnPluginTab(const FSpawnTabArgs& SpawnTabArgs)
{
	auto DockTab = SNew(SDockTab)
		.TabRole(ETabRole::NomadTab);

	TSubclassOf<UEditorUtilityWidget> WidgetClass;
	const FAssetRegistryModule & AssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	AssetRegistry.Get().ScanFilesSynchronous(TArray<FString>{"/StableDiffusionTools"});
	auto Data = AssetRegistry.Get().GetAssetByObjectPath("/StableDiffusionTools/UI/Widgets/BP_StableDiffusionViewportWidget.BP_StableDiffusionViewportWidget");
	// check if valid:

	if (Data.AssetName.ToString().Equals(TEXT("BP_StableDiffusionViewportWidget"), ESearchCase::CaseSensitive))
	{
		UBlueprint* BP = Cast<UBlueprint>(Data.GetAsset());
		if (BP && BP->GeneratedClass.Get())
		{
			if (!BP->GeneratedClass.Get()->HasAnyClassFlags(CLASS_Deprecated | CLASS_Hidden)) {
				WidgetClass = BP->GeneratedClass.Get();
			}
		}
	}
	


	UWorld* World = GEditor->GetEditorWorldContext().World();
	check(World);

	UEditorUtilityWidget* CreatedUMGWidget = CreateWidget<UStableDiffusionViewportWidget>(World, WidgetClass);
	if (CreatedUMGWidget)
	{
		DockTab->SetContent(CreatedUMGWidget->TakeWidget());
	}

	return DockTab;
}

void FStableDiffusionToolsModule::PluginButtonClicked()
{
	FGlobalTabmanager::Get()->TryInvokeTab(StableDiffusionToolsTabName);
}

void FStableDiffusionToolsModule::RegisterMenus()
{
	// Owner will be used for cleanup in call to UToolMenus::UnregisterOwner
	FToolMenuOwnerScoped OwnerScoped(this);

	{
		UToolMenu* Menu = UToolMenus::Get()->ExtendMenu("LevelEditor.MainMenu.Window");
		{
			FToolMenuSection& Section = Menu->FindOrAddSection("WindowLayout");
			Section.AddMenuEntryWithCommandList(FStableDiffusionToolsCommands::Get().OpenPluginWindow, PluginCommands);
		}
	}

	{
		UToolMenu* ToolbarMenu = UToolMenus::Get()->ExtendMenu("LevelEditor.LevelEditorToolBar");
		{
			FToolMenuSection& Section = ToolbarMenu->FindOrAddSection("Settings");
			{
				FToolMenuEntry& Entry = Section.AddEntry(FToolMenuEntry::InitToolBarButton(FStableDiffusionToolsCommands::Get().OpenPluginWindow));
				Entry.SetCommandList(PluginCommands);
			}
		}
	}
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FStableDiffusionToolsModule, StableDiffusionTools)