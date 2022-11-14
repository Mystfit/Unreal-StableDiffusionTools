// Copyright Epic Games, Inc. All Rights Reserved.

#include "StableDiffusionToolsEditor.h"
#include "StableDiffusionToolsStyle.h"
#include "StableDiffusionToolsCommands.h"

#include "LevelEditor.h"
#include "Widgets/Docking/SDockTab.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Text/STextBlock.h"
#include "ToolMenus.h"
#include "Engine/ObjectLibrary.h"
#include "AssetRegistryModule.h"
#include "EditorUtilityWidget.h"

static const FName StableDiffusionToolsTabName("Stable Diffusion Tools");
static const FName StableDiffusionDependencyInstallerTabName("Stable Diffusion Dependency Installer");

#define LOCTEXT_NAMESPACE "FStableDiffusionToolsEditorModule"

void FStableDiffusionToolsEditorModule::StartupModule()
{
	FStableDiffusionToolsStyle::Initialize();
	FStableDiffusionToolsStyle::ReloadTextures();
	FStableDiffusionToolsCommands::Register();
		
	PluginCommands = MakeShareable(new FUICommandList);
	PluginCommands->MapAction(
		FStableDiffusionToolsCommands::Get().OpenPluginWindow,
		FExecuteAction::CreateRaw(this, &FStableDiffusionToolsEditorModule::PluginButtonClicked),
		FCanExecuteAction());

	UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FStableDiffusionToolsEditorModule::RegisterMenus));
	
	// Main plugin UI tab
	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(StableDiffusionToolsTabName, FOnSpawnTab::CreateRaw(this, &FStableDiffusionToolsEditorModule::OnSpawnPluginTab))
		.SetDisplayName(LOCTEXT("FStableDiffusionToolsTabTitle", "Stable Diffusion Tools"))
		.SetMenuType(ETabSpawnerMenuType::Hidden);

	// Dependency installer tab
	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(StableDiffusionDependencyInstallerTabName, FOnSpawnTab::CreateRaw(this, &FStableDiffusionToolsEditorModule::OnSpawnDependencyInstallerTab))
		.SetDisplayName(LOCTEXT("FStableDiffusionToolsDependencyInstallerTabTitle", "Stable Diffusion Dependency Installer"))
		.SetMenuType(ETabSpawnerMenuType::Hidden);
}

void FStableDiffusionToolsEditorModule::ShutdownModule()
{
	UToolMenus::UnRegisterStartupCallback(this);

	UToolMenus::UnregisterOwner(this);

	FStableDiffusionToolsStyle::Shutdown();

	FStableDiffusionToolsCommands::Unregister();

	FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(StableDiffusionToolsTabName);
}

TSharedRef<SDockTab> FStableDiffusionToolsEditorModule::OnSpawnPluginTab(const FSpawnTabArgs& SpawnTabArgs)
{
	TSharedPtr<SDockTab> DockTab = SNew(SDockTab).TabRole(ETabRole::NomadTab);// .ShouldAutosize(true);

	const FAssetRegistryModule & AssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	FString EditorUIPackage = "/StableDiffusionTools/UI/Widgets/BP_StableDiffusionViewportWidget";
	AssetRegistry.Get().ScanFilesSynchronous({ FPackageName::LongPackageNameToFilename(EditorUIPackage, FPackageName::GetAssetPackageExtension())});
	auto Data = AssetRegistry.Get().GetAssetByObjectPath(FName(EditorUIPackage.Append(".BP_StableDiffusionViewportWidget")));
	check(Data.IsValid());

	TSubclassOf<UEditorUtilityWidget> WidgetClass;
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

	UStableDiffusionViewportWidget::CreateViewportWidget(DockTab, WidgetClass);
	

	return DockTab.ToSharedRef();
}


TSharedRef<SDockTab> FStableDiffusionToolsEditorModule::OnSpawnDependencyInstallerTab(const FSpawnTabArgs& SpawnTabArgs)
{
	auto DockTab = SNew(SDockTab).TabRole(ETabRole::NomadTab);// .ShouldAutosize(true);

	const FAssetRegistryModule& AssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	FString EditorUIPackage = "/StableDiffusionTools/UI/Widgets/BP_StableDiffusionDependencyInstallerWidget";
	AssetRegistry.Get().ScanFilesSynchronous({ FPackageName::LongPackageNameToFilename(EditorUIPackage, FPackageName::GetAssetPackageExtension()) });
	auto Data = AssetRegistry.Get().GetAssetByObjectPath(FName(EditorUIPackage.Append(".BP_StableDiffusionDependencyInstallerWidget")));
	check(Data.IsValid());

	TSubclassOf<UEditorUtilityWidget> WidgetClass;
	if (Data.AssetName.ToString().Equals(TEXT("BP_StableDiffusionDependencyInstallerWidget"), ESearchCase::CaseSensitive))
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

	UEditorUtilityWidget* CreatedUMGWidget = CreateWidget<UEditorUtilityWidget>(GEditor->GetEditorWorldContext().World(), WidgetClass);
	if (CreatedUMGWidget)
	{
		if (CreatedUMGWidget)
		{
			// Editor Utility is flagged as transient to prevent from dirty the World it's created in when a property added to the Utility Widget is changed
			CreatedUMGWidget->SetFlags(RF_Transient);
			FLevelEditorModule& LevelEditor = FModuleManager::LoadModuleChecked<FLevelEditorModule>("LevelEditor");
			LevelEditor.OnMapChanged().AddLambda([CreatedUMGWidget, DockTab, WidgetClass](UWorld* World, EMapChangeType MapChangeType) {
				if (MapChangeType == EMapChangeType::TearDownWorld)
				{
					// We need to Delete the UMG widget if we are tearing down the World it was built with.
					if (CreatedUMGWidget && World == CreatedUMGWidget->GetWorld())
					{
						DockTab->SetContent(SNullWidget::NullWidget);
						CreatedUMGWidget->Rename(nullptr, GetTransientPackage());
					}
				}
				else if (MapChangeType != EMapChangeType::SaveMap) {
					//UEditorUtilityWidget* ReplacementUMGWidget = CreateWidget<UEditorUtilityWidget>(GEditor->GetEditorWorldContext().World(), WidgetClass);
					//DockTab->SetContent(ReplacementUMGWidget->TakeWidget());
				}
			});
		}
		DockTab->SetContent(CreatedUMGWidget->TakeWidget());
	}

	return DockTab;
}


void FStableDiffusionToolsEditorModule::PluginButtonClicked()
{
	FGlobalTabmanager::Get()->TryInvokeTab(StableDiffusionToolsTabName);
}

void FStableDiffusionToolsEditorModule::OpenDependencyInstallerWindow() 
{
	FGlobalTabmanager::Get()->TryInvokeTab(StableDiffusionDependencyInstallerTabName);
}

void FStableDiffusionToolsEditorModule::RegisterMenus()
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
	
IMPLEMENT_MODULE(FStableDiffusionToolsEditorModule, StableDiffusionToolsEditor)