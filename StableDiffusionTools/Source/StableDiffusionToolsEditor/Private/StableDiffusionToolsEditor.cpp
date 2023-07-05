// Copyright Epic Games, Inc. All Rights Reserved.

#include "StableDiffusionToolsEditor.h"
#include "StableDiffusionToolsStyle.h"
#include "StableDiffusionGenerationAssetActions.h"
#include "StableDiffusionGenerationAssetThumbnailRenderer.h"
#include "StableDiffusionToolsCommands.h"
#include "SDDependencyInstallerWidget.h"
#include "StableDiffusionSubsystem.h"
#include "LevelEditor.h"
#include "Widgets/Docking/SDockTab.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Text/STextBlock.h"
#include "ToolMenus.h"
#include "Engine/ObjectLibrary.h"
#include "AssetRegistryModule.h"
#include "Engine/AssetManager.h"
#include "EditorUtilityWidget.h"
#include "EditorAssetLibrary.h"
#include "EditorClassUtils.h"
#include "LayerProcessorBaseTypeActions.h"
#include "LayerProcessorOptionsCustomization.h"
#include "EditorUtilitySubsystem.h"

static const FName StableDiffusionToolsTabName("Stable Diffusion Tools");
static const FName StableDiffusionDependencyInstallerTabName("Stable Diffusion Dependency Installer");

#define LOCTEXT_NAMESPACE "FStableDiffusionToolsEditorModule"

void FStableDiffusionToolsEditorModule::StartupModule()
{
	FStableDiffusionToolsStyle::Initialize();
	FStableDiffusionToolsStyle::ReloadTextures();
	FStableDiffusionToolsCommands::Register();
	
	// Register commands
	PluginCommands = MakeShareable(new FUICommandList);
	PluginCommands->MapAction(
		FStableDiffusionToolsCommands::Get().OpenPluginWindow,
		FExecuteAction::CreateRaw(this, &FStableDiffusionToolsEditorModule::PluginButtonClicked),
		FCanExecuteAction());
	PluginCommands->MapAction(
		FStableDiffusionToolsCommands::Get().OpenDependenciesWindow,
		FExecuteAction::CreateRaw(this, &FStableDiffusionToolsEditorModule::OpenDependencyInstallerWindow),
		FCanExecuteAction());
	PluginCommands->MapAction(
		FStableDiffusionToolsCommands::Get().OpenModelToolsWindow,
		FExecuteAction::CreateRaw(this, &FStableDiffusionToolsEditorModule::OpenModelToolsWindow),
		FCanExecuteAction());

	// Create menus
	{
		TSharedPtr<FExtender> NewMenuExtender = MakeShareable(new FExtender);
		NewMenuExtender->AddMenuExtension("LevelEditor",
			EExtensionHook::After,
			PluginCommands,
			FMenuExtensionDelegate::CreateRaw(this, &FStableDiffusionToolsEditorModule::AddMenuEntry));

		FLevelEditorModule& LevelEditorModule = FModuleManager::LoadModuleChecked<FLevelEditorModule>("LevelEditor");
		LevelEditorModule.GetMenuExtensibilityManager()->AddExtender(NewMenuExtender);
	}

	RegisterCustomAssetActions();
	RegisterDetailCustomization();

	// Register thumbnails
	UThumbnailManager::Get().RegisterCustomRenderer(UStableDiffusionImageResultAsset::StaticClass(), UStableDiffusionGenerationAssetThumbnailRenderer::StaticClass());

	/*
	// Register assets
	IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
	AssetTools.RegisterAssetTypeActions(MakeShareable(new FAssetTypeActions_LayerProcessorBase));

	// Automatically register existing assets of your primary data asset class
	FTopLevelAssetPath BaseClassPathName = ULayerProcessorBase::StaticClass()->GetClassPathName();
	UClass* BlueprintClass = UBlueprint::StaticClass();

	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));

	// Use the asset registry to get the set of all class names deriving from Base
	TSet< FTopLevelAssetPath > DerivedPathNames;
	{
		TArray< FTopLevelAssetPath > BasePathNames;
		BasePathNames.Add(BaseClassPathName);

		TSet< FTopLevelAssetPath > Excluded;
		AssetRegistryModule.Get().GetDerivedClassNames(BasePathNames, Excluded, DerivedPathNames);
	}

	TArray<FAssetData> ObjectList;
	AssetRegistryModule.Get().GetAssetsByClass(BlueprintClass->GetClassPathName(), ObjectList);
	for (auto ObjIter = ObjectList.CreateConstIterator(); ObjIter; ++ObjIter)
	{
		const FAssetData& Asset = *ObjIter;
		FString Filename = Asset.GetObjectPathString();

		// Get the the class this blueprint generates
		auto AssetTag = Asset.TagsAndValues.FindTag(TEXT("GeneratedClass"));
		if (AssetTag.IsSet())
		{
			// Convert path to just the name part
			const FTopLevelAssetPath ClassPathName(FPackageName::ExportTextPathToObjectPath(*AssetTag.AsString()));

			// Check if this class is in the derived set
			if (!DerivedPathNames.Contains(ClassPathName))
			{
				continue;
			}
			UAssetManager::Get().LoadPrimaryAsset(Asset.GetPrimaryAssetId());
		}
	}
	*/
}

void FStableDiffusionToolsEditorModule::ShutdownModule()
{
	UToolMenus::UnRegisterStartupCallback(this);
	UToolMenus::UnregisterOwner(this);

	FStableDiffusionToolsStyle::Shutdown();
	FStableDiffusionToolsCommands::Unregister();
}

void FStableDiffusionToolsEditorModule::PluginButtonClicked()
{
	FString DependencyBPPath = "/StableDiffusionTools/UI/Widgets/BP_StableDiffusionViewportWidget.BP_StableDiffusionViewportWidget";
	CreatePanel(DependencyBPPath, "SD Tools");
}

void FStableDiffusionToolsEditorModule::OpenDependencyInstallerWindow() 
{
	FString DependencyBPPath = "/StableDiffusionTools/UI/Widgets/BP_StableDiffusionDependencyInstallerWidget.BP_StableDiffusionDependencyInstallerWidget";
	CreatePanel(DependencyBPPath, "SD Dependencies");
}

void FStableDiffusionToolsEditorModule::OpenModelToolsWindow()
{
	FString DependencyBPPath = "/StableDiffusionTools/UI/Widgets/BP_StableDiffusionModelUtilities.BP_StableDiffusionModelUtilities";
	CreatePanel(DependencyBPPath, "SD Model Tools");
}

void FStableDiffusionToolsEditorModule::CreatePanel(const FString& BlueprintAssetPath, const FString& PanelLabel)
{
	UEditorUtilitySubsystem* EUSubsystem = GEditor->GetEditorSubsystem<UEditorUtilitySubsystem>();
	UEditorUtilityWidgetBlueprint* EditorWidget = LoadObject<UEditorUtilityWidgetBlueprint>(NULL, *BlueprintAssetPath, NULL, LOAD_Verify, NULL);
	EUSubsystem->SpawnAndRegisterTab(EditorWidget);

	// Find created tab so we can rename it
	FName TabID;
	EUSubsystem->SpawnAndRegisterTabAndGetID(EditorWidget, TabID);
	FLevelEditorModule& LevelEditorModule = FModuleManager::GetModuleChecked<FLevelEditorModule>(TEXT("LevelEditor"));
	TSharedPtr<FTabManager> LevelEditorTabManager = LevelEditorModule.GetLevelEditorTabManager();
	if (auto Tab = LevelEditorTabManager->FindExistingLiveTab(TabID)) {
		Tab->SetLabel(FText::FromString(PanelLabel));
	}
}

void FStableDiffusionToolsEditorModule::AddMenuEntry(FMenuBuilder& MenuBuilder)
{
	MenuBuilder.BeginSection("StableDiffusionToolsMenu", TAttribute<FText>(FText::FromString("Stable Diffusion Tools")));
	MenuBuilder.AddMenuEntry(FStableDiffusionToolsCommands::Get().OpenPluginWindow);
	MenuBuilder.AddMenuEntry(FStableDiffusionToolsCommands::Get().OpenDependenciesWindow);
	MenuBuilder.AddMenuEntry(FStableDiffusionToolsCommands::Get().OpenModelToolsWindow);
	MenuBuilder.EndSection();
}

void FStableDiffusionToolsEditorModule::RegisterCustomAssetActions()
{
	IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
	TSharedRef<FStableDiffusionImageResultAssetActions> Action = MakeShareable(new FStableDiffusionImageResultAssetActions());
	AssetTools.RegisterAssetTypeActions(Action);
}

void FStableDiffusionToolsEditorModule::RegisterDetailCustomization()
{
	const FName PropertyEditor("PropertyEditor");
	FPropertyEditorModule& PropertyModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>(PropertyEditor);
	
	//PropertyModule.RegisterCustomPropertyTypeLayout(FLayerProcessorContext::StaticStruct()->GetFName(), FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FLayerDataCustomization::MakeInstance));
	//PropertyModule.RegisterCustomPropertyTypeLayout(ULayerProcessorOptions::StaticClass()->GetFName(), FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FLayerProcessorOptionsCustomization::MakeInstance));
}



#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FStableDiffusionToolsEditorModule, StableDiffusionToolsEditor)