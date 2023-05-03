// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Engine/Texture2D.h"
#include "StableDiffusionImageResult.h"
#include "StableDiffusionGenerationOptions.h"
#include "StableDiffusionBridge.generated.h"

DECLARE_MULTICAST_DELEGATE_FiveParams(FImageProgress, int32, int32, float, FIntPoint, const TArray<FColor>&);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FiveParams(FImageProgressEx, int32, Step, int32, Timestep, float, Progress, FIntPoint, size, const TArray<FColor>&, PixelData);

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
    FString GetTokenWebsiteHint();

	//UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "StableDiffusion|Bridge")
 //   USDBridgeToken* CachedToken;

    UFUNCTION(BlueprintCallable, Category = "StableDiffusion|Bridge")
    void SaveProperties();

    /** Python stable diffusion implementable functions */

    UFUNCTION(BlueprintImplementableEvent, Category = "StableDiffusion|Bridge")
    bool ModelExists(const FString& ModelName) const;

    UFUNCTION(BlueprintImplementableEvent, Category = "StableDiffusion|Bridge")
    bool InitModel(const FStableDiffusionModelOptions& NewModelOptions, bool AllowNsfw, EPaddingMode PaddingMode);

    UFUNCTION(BlueprintImplementableEvent, Category = "StableDiffusion|Bridge")
    void ReleaseModel();

    UPROPERTY(BlueprintReadOnly, Category = "StableDiffusion|Model")
    bool ModelInitialising = false;

    UFUNCTION(BlueprintImplementableEvent, Category = "StableDiffusion|Bridge")
    FStableDiffusionImageResult GenerateImageFromStartImage(const FStableDiffusionInput& InputOptions) const;

    UFUNCTION(BlueprintImplementableEvent, Category = "StableDiffusion|Bridge")
    void StopImageGeneration();

    UFUNCTION(BlueprintImplementableEvent, Category = "StableDiffusion|Bridge")
    void StartUpsample();

    UFUNCTION(BlueprintImplementableEvent, Category = "StableDiffusion|Bridge")
    FStableDiffusionImageResult UpsampleImage(const FStableDiffusionImageResult& input_result) const;

    UFUNCTION(BlueprintImplementableEvent, Category = "StableDiffusion|Bridge")
    void StopUpsample();

    UFUNCTION(BlueprintCallable, Category = "StableDiffusion|Bridge")
    void UpdateImageProgress(FString prompt, int32 step, int32 timestep, float progress, int32 width, int32 height, const TArray<FColor>& FrameColors);

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "StableDiffusion|Bridge")
    FStableDiffusionModelOptions ModelOptions;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "StableDiffusion|Bridge")
    EModelStatus ModelStatus = EModelStatus::Unloaded;

    FImageProgress OnImageProgress;
    FImageProgressEx OnImageProgressEx;
};
