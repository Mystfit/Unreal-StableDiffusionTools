// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Engine/Texture2D.h"
#include "StableDiffusionImageResult.h"
#include "StableDiffusionBridge.generated.h"

DECLARE_MULTICAST_DELEGATE_FourParams(FImageProgress, int32, int32, FIntPoint, const TArray<FColor>&);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FImageProgressEx, int32, Step, int32, Timestep, FIntPoint, size, const TArray<FColor>&, PixelData);

/**
 * 
 */
UCLASS()
class STABLEDIFFUSIONTOOLS_API UStableDiffusionBridge : public UObject
{
	GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = Python)
    static UStableDiffusionBridge* Get();


    /** Python stable diffusion implementable functions */

    UFUNCTION(BlueprintImplementableEvent, Category = StableDiffusion)
    FString GetPythonID();

    UFUNCTION(BlueprintImplementableEvent, Category = StableDiffusion)
    bool InitModel();

    UFUNCTION(BlueprintImplementableEvent, Category = StableDiffusion)
    FStableDiffusionImageResult GenerateImageFromStartImage(const FString& prompt, int32 InFrameWidth, int32 InFrameHeight, int32 OutFrameWidth, int32 OutFrameHeight, const TArray<FColor>& GuideFrame, const TArray<FColor>& MaskFrame, float Strength, int32 Iterations, int32 Seed) const;

    UFUNCTION(BlueprintCallable)
    void UpdateImageProgress(FString prompt, int32 step, int32 timestep, int32 width, int32 height, const TArray<FColor>& FrameColors);

    FImageProgress OnImageProgress;
    FImageProgressEx OnImageProgressEx;
};
