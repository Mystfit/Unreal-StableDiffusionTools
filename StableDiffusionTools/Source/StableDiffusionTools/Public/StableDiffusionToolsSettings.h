// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "StableDiffusionBridge.h"
#include "StableDiffusionToolsSettings.generated.h"

/**
 *
 */
UCLASS(config = Engine, defaultconfig)
class STABLEDIFFUSIONTOOLS_API UStableDiffusionToolsSettings : public UObject
{
	GENERATED_BODY()

public:
	/** Gets the class of the Stable Diffusion generator bridge that will be constructed.*/
	TSubclassOf<UStableDiffusionBridge> GetGeneratorType() const;

	/** Gets the class of the Stable Diffusion generator bridge that will be constructed.*/
	TMap<FName, FString> GetGeneratorTokens() const;

	void AddGeneratorToken(const FName& Generator);

	/**
	* Automatically load all bridge scripts when the editor starts. Disable this if you are having issues with updating python bridge script dependencies that are loaded into memory.
	**/
	UPROPERTY(BlueprintReadWrite, config, EditAnywhere, Category = "Options")
	bool AutoLoadBridgeScripts = true;

	/**
	* Automatically load all bridge scripts when the editor starts. Disable this if you are having issues with updating python bridge script dependencies that are loaded into memory.
	**/
	UPROPERTY(BlueprintReadWrite, config, EditAnywhere, Category = "Options")
	bool AutoUpdateDependenciesOnStartup = false;

	/**
	* Remove all downloaded python dependencies when the editor next starts.
	**/
	UPROPERTY(BlueprintReadWrite, config, EditAnywhere, Category = "Options")
	bool ClearDependenciesOnEditorRestart = false;
private:

	/** Stable Diffusion generators are local or remote providers that will create an AI-generated image based on a prompt and input image.*/
	UPROPERTY(config, EditAnywhere, Category = "Options")
	TSubclassOf<UStableDiffusionBridge> GeneratorType;

	UPROPERTY(config, EditAnywhere, Category = "Options")
	TMap<FName, FString> GeneratorTokens;
};