// Fill out your copyright notice in the Description page of Project Settings.

#include "SDEditorBlueprintLibrary.h"
#include "StableDiffusionToolsEditor.h"

void USDEditorBlueprintLibrary::OpenDependencyInstaller()
{
	FStableDiffusionToolsEditorModule& SDEditorModule = FModuleManager::GetModuleChecked<FStableDiffusionToolsEditorModule>("StableDiffusionToolsEditor");
	SDEditorModule.OpenDependencyInstallerWindow();
}
