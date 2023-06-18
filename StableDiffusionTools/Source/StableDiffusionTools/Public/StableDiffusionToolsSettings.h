// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "IDetailCustomization.h"
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

	/** Gets the class of the Stable Diffusion generator bridge that will be constructed.*/
	FDirectoryPath GetLORASavePath() const;

	void AddGeneratorToken(const FName& Generator);

private:

	/** Stable Diffusion generators are local or remote providers that will create an AI-generated image based on a prompt and input image.*/
	UPROPERTY(config, EditAnywhere, Category = "Options")
	TSubclassOf<UStableDiffusionBridge> GeneratorType;

	UPROPERTY(config, EditAnywhere, Category = "Options")
	TMap<FName, FString> GeneratorTokens;

	UPROPERTY(config, EditAnywhere, meta=(DisplayName="LORA Save Path", Category = "Options"))
	FDirectoryPath LORASavePath;
};


class FStableDiffusionToolsSettingsDetails : public IDetailCustomization
{
public:
	/** Makes a new instance of this detail layout class for a specific detail view requesting it */
	static TSharedRef<IDetailCustomization> MakeInstance();

	/** IDetailCustomization interface */
	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;
};
