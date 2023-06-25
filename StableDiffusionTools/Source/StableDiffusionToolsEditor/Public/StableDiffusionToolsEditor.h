// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"
#include "StableDiffusionViewportWidget.h"
#include "SDDependencyInstallerWidget.h"

class FToolBarBuilder;
class FMenuBuilder;

class STABLEDIFFUSIONTOOLSEDITOR_API FStableDiffusionToolsEditorModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	bool HandleSettingsSaved();
	
	/** This function will be bound to Command (by default it will bring up plugin window) */
	void PluginButtonClicked();
	void OpenDependencyInstallerWindow();
	void OpenModelToolsWindow();

	void CreatePanel(const FString& BlueprintAssetPath, const FString& PanelLabel);

	void RegisterCustomAssetActions();
	void RegisterDetailCustomization();
	
private:
	void AddMenuEntry(FMenuBuilder& MenuBuilder);

private:
	TSharedPtr<class FUICommandList> PluginCommands;
};
