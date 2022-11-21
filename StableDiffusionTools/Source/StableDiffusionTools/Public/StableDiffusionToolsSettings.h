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
private:

	/** Stable Diffusion generators are local or remote providers that will create an AI-generated image based on a prompt and input image.*/
	UPROPERTY(config, EditAnywhere, Category = "Options")
	TSubclassOf<UStableDiffusionBridge> GeneratorType;
};