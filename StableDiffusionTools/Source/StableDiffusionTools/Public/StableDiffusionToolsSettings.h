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
	UFUNCTION(BlueprintCallable, meta = (Category = "Options"))
	TSubclassOf<UStableDiffusionBridge> GetGeneratorType() const;

	/** Authentication tokens for Stable Diffusion generator bridges.*/
	UFUNCTION(BlueprintCallable, meta = (Category = "Options"))
	TMap<FName, FString> GetGeneratorTokens() const;

	/** Gets the location where base models will be saved by default.*/
	UFUNCTION(BlueprintCallable, meta = (Category = "Options"))
	FDirectoryPath GetModelDownloadPath();
	
	UFUNCTION(BlueprintCallable, meta = (Category = "Options"))
	bool GetUseOverridePythonSitePackagesPath() const;

	UFUNCTION(BlueprintCallable, meta = (Category = "Options"))
	bool GetFreezeDependencies() const;

	/** Gets the location of an override python site-packages*/
	UFUNCTION(BlueprintCallable, meta = (Category = "Options"))
	FDirectoryPath GetPythonSitePackagesOverridePath();

	void AddGeneratorToken(const FName& Generator);

private:

	/** Stable Diffusion generators are local or remote providers that will create an AI-generated image based on a prompt and input image.*/
	UPROPERTY(config, EditAnywhere, Category = "Options")
	TSubclassOf<UStableDiffusionBridge> GeneratorType;

	/** Stable Diffusion generator tokens to allow generator bridges to access web APIs.*/
	UPROPERTY(config, EditAnywhere, Category = "Options")
	TMap<FName, FString> GeneratorTokens;

	/** Path to download model files to. */
	UPROPERTY(config, EditAnywhere, AdvancedDisplay, meta = (DisplayName = "Model download path", Category = "Options"))
	FDirectoryPath ModelDownloadPath;

	/** Use an overriden python site-packages folder. */
	UPROPERTY(config, EditAnywhere, AdvancedDisplay, meta = (DisplayName = "Use custom python packages path", Category = "Options"))
	bool bOverridePythonSitePackagesPath;

	/** Downloads python dependencies to "PLUGIN_DIR/FrozenPythonDependencies". You can safely leave this setting alone unless you're packaging the editor plugin for distribution to other editors. */
	UPROPERTY(config, EditAnywhere, AdvancedDisplay, meta = (DisplayName = "Freeze python dependencies", Category = "Options"))
	bool bFreezeDependencies;

	/** Overriden python site-packages folder. */
	UPROPERTY(config, EditAnywhere, AdvancedDisplay, meta = (DisplayName = "Python package installation directory", Category = "Options", EditCondition = "bOverridePythonSitePackagesPath", EditConditionHides))
	FDirectoryPath PythonSitePackagesPath;
};


class FStableDiffusionToolsSettingsDetails : public IDetailCustomization
{
public:
	/** Makes a new instance of this detail layout class for a specific detail view requesting it */
	static TSharedRef<IDetailCustomization> MakeInstance();

	/** IDetailCustomization interface */
	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;
};
