// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Engine/Texture2D.h"
#include "StableDiffusionBridge.generated.h"

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
    void InitModel();

    UFUNCTION(BlueprintImplementableEvent, Category = StableDiffusion)
    TArray<FColor> GenerateImageFromStartImage(const FString& prompt, int32 FrameWidth, int32 Height, const TArray<FColor>& GuideFrame, const TArray<FColor>& MaskFrame, float Strength, int32 Iterations, int32 Seed) const;

    UFUNCTION(BlueprintCallable)
    UTexture2D* ColorBufferToTexture(const FString& FrameName, const TArray<FColor>& FrameColors, const FIntPoint& FrameSize);
};
