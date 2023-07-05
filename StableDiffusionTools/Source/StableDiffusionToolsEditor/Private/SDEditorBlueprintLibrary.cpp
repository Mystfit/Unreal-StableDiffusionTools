// Fill out your copyright notice in the Description page of Project Settings.

#include "SDEditorBlueprintLibrary.h"
#include "Engine/AssetManager.h"
#include "StableDiffusionToolsEditor.h"

void USDEditorBlueprintLibrary::OpenDependencyInstaller()
{
	FStableDiffusionToolsEditorModule& SDEditorModule = FModuleManager::GetModuleChecked<FStableDiffusionToolsEditorModule>("StableDiffusionToolsEditor");
	SDEditorModule.OpenDependencyInstallerWindow();
}

void USDEditorBlueprintLibrary::OpenModelTools()
{
	FStableDiffusionToolsEditorModule& SDEditorModule = FModuleManager::GetModuleChecked<FStableDiffusionToolsEditorModule>("StableDiffusionToolsEditor");
	SDEditorModule.OpenModelToolsWindow();
}

void USDEditorBlueprintLibrary::ScanForLayerProcessorAssets()
{
	UAssetManager::Get().ScanPathForPrimaryAssets(FName("LayerProcessorBase"), "/StableDiffusionTools/LayerProcessors", UPrimaryDataAsset::StaticClass(), true);
	//UAssetManager::Get().ReinitializeFromConfig();
}
