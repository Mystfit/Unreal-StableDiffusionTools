// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"
#include "StableDiffusionViewportWidget.h"

class FToolBarBuilder;
class FMenuBuilder;

class FStableDiffusionToolsEditorModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
	
	/** This function will be bound to Command (by default it will bring up plugin window) */
	void PluginButtonClicked();

	
private:

	void RegisterMenus();

	TSharedRef<class SDockTab> OnSpawnPluginTab(const class FSpawnTabArgs& SpawnTabArgs);

	UPROPERTY(EditAnywhere, Category = "StableDiffusion|UI")
	TSubclassOf<UStableDiffusionViewportWidget> ViewportWidget;


private:
	TSharedPtr<class FUICommandList> PluginCommands;
};
