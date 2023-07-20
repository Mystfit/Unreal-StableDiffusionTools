// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Engine/Texture2D.h"
#include "StableDiffusionImageResult.h"
#include "StableDiffusionGenerationOptions.h"
#include "StableDiffusionBridge.generated.h"

DECLARE_MULTICAST_DELEGATE_FiveParams(FImageProgress, int32, int32, float, FIntPoint, UTexture2D* Texture);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FiveParams(FImageProgressEx, int32, Step, int32, Timestep, float, Progress, FIntPoint, size, UTexture2D*, Texture);

//UCLASS(PerObjectConfig, Config = "Engine")
//class USDBridgeToken : public UObject
//{
//    GENERATED_BODY()
//public:
//    UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category = "StableDiffusion|Token")
//    FString Token;
//};

/**
 * 
 */
UCLASS(Config="Engine")
class STABLEDIFFUSIONTOOLS_API UStableDiffusionBridge : public UObject
{
	GENERATED_BODY()

public:
    UStableDiffusionBridge(const FObjectInitializer& initializer);

    UFUNCTION(BlueprintCallable, Category = "StableDiffusion|Bridge")
    UStableDiffusionBridge* Get();

    UFUNCTION(BlueprintCallable, Category = "StableDiffusion|Bridge")
    bool LoginUsingToken(const FString& Token);

    UFUNCTION(BlueprintCallable, Category = "StableDiffusion|Bridge")
    FString GetToken();

    UFUNCTION(BlueprintImplementableEvent, Category = "StableDiffusion|Bridge")
    bool GetRequiresToken();

    UFUNCTION(BlueprintImplementableEvent, Category = "StableDiffusion|Bridge")
    FString GetTokenWebsiteHint();

    UFUNCTION(BlueprintCallable, Category = "StableDiffusion|Bridge")
    FDirectoryPath GetSettingsModelSavePath();

    UFUNCTION(BlueprintCallable, Category = "StableDiffusion|Bridge")
    void SaveProperties();

    /** Python stable diffusion implementable functions */

    UFUNCTION(BlueprintImplementableEvent, Category = "StableDiffusion|Bridge")
    bool ModelExists(const FString& ModelName) const;

    UFUNCTION(BlueprintImplementableEvent, Category = "StableDiffusion|Bridge")
    bool ConvertRawModel(UStableDiffusionModelAsset* InModelAsset, bool DeleteOriginal = true);

    UFUNCTION(BlueprintImplementableEvent, Category = "StableDiffusion|Bridge")
    FStableDiffusionModelInitResult InitModel(const FStableDiffusionModelOptions& NewModelOptions, UStableDiffusionPipelineAsset* NewPipelineAsset, UStableDiffusionLORAAsset* LoraAsset = nullptr, UStableDiffusionTextualInversionAsset* TextualInversionAsset = nullptr, const TArray<FLayerProcessorContext>& Layers = TArray<FLayerProcessorContext>(), bool AllowNsfw = false, EPaddingMode PaddingMode = EPaddingMode::zeros);

    UFUNCTION(BlueprintImplementableEvent, Category = "StableDiffusion|Bridge")
    void ReleaseModel();

	UFUNCTION(BlueprintImplementableEvent, Category = "StableDiffusion|Bridge")
	TArray<FString> AvailableSchedulers();

    UFUNCTION(BlueprintImplementableEvent, Category = "StableDiffusion|Bridge")
    FString GetScheduler() const;

    UPROPERTY(BlueprintReadOnly, Category = "StableDiffusion|Model")
    bool ModelInitialising = false;

    UFUNCTION(BlueprintImplementableEvent, Category = "StableDiffusion|Bridge")
    FStableDiffusionImageResult GenerateImageFromStartImage(const FStableDiffusionInput& InputOptions, UTexture* OutTexture, UTexture* PreviewTexture) const;

    UFUNCTION(BlueprintImplementableEvent, Category = "StableDiffusion|Bridge")
    void StopImageGeneration();

    UFUNCTION(BlueprintImplementableEvent, Category = "StableDiffusion|Bridge")
    void StartUpsample();

    UFUNCTION(BlueprintImplementableEvent, Category = "StableDiffusion|Bridge")
    FStableDiffusionImageResult UpsampleImage(const FStableDiffusionImageResult& input_result, UTexture2D* OutTexture) const;

    UFUNCTION(BlueprintImplementableEvent, Category = "StableDiffusion|Bridge")
    void StopUpsample();

    UFUNCTION(BlueprintCallable, Category = "StableDiffusion|Bridge")
    void UpdateImageProgress(FString prompt, int32 step, int32 timestep, float progress, int32 width, int32 height, UTexture2D* Texture);

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "StableDiffusion|Bridge")
    FStableDiffusionModelOptions ModelOptions;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "StableDiffusion|Bridge")
	UStableDiffusionPipelineAsset* PipelineAsset;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "StableDiffusion|Bridge")
    UStableDiffusionLORAAsset* LORAAsset;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "StableDiffusion|Bridge")
    UStableDiffusionTextualInversionAsset* CachedTextualInversionAsset;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "StableDiffusion|Bridge")
    FStableDiffusionModelInitResult ModelStatus;

    FImageProgress OnImageProgress;
    FImageProgressEx OnImageProgressEx;
};
