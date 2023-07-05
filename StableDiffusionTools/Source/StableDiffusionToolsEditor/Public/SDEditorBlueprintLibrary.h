// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "SDEditorBlueprintLibrary.generated.h"

/**
 * 
 */
UCLASS()
class STABLEDIFFUSIONTOOLSEDITOR_API USDEditorBlueprintLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:

	UFUNCTION(BlueprintCallable, Category = "Dependencies")
	static void OpenDependencyInstaller();

	UFUNCTION(BlueprintCallable, Category = "ModelTools")
	static void OpenModelTools();

	UFUNCTION(BlueprintCallable, Category = "ModelTools")
	static void ScanForLayerProcessorAssets();
};
