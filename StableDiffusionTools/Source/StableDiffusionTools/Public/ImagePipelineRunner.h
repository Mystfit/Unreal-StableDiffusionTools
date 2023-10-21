// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "StableDiffusionImageResult.h"
#include "StableDiffusionGenerationOptions.h"
#include "ImagePipelineRunner.generated.h"

/**
 * 
 */

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FImageStageResult, FStableDiffusionPipelineImageResult, StageResult, UStableDiffusionImageResultAsset*, ResultAsset);


UCLASS(Blueprintable)
class STABLEDIFFUSIONTOOLS_API UImagePipelineRunner : public UBlueprintAsyncActionBase
{
	GENERATED_UCLASS_BODY()
public:

	UFUNCTION(BlueprintCallable, meta = (Category = "Async", BlueprintInternalUseOnly = "true"))
	static UImagePipelineRunner* RunImagePipeline(
		UStableDiffusionImageResultAsset* OutAsset, 
		TArray<UImagePipelineStageAsset*> Stages, 
		FStableDiffusionInput Input, 
		EInputImageSource ImageSourceType, 
		bool AllowNSFW = false, 
		int32 SeamlessMode = 0x00, 
		bool SaveLayers = false
	);

	// UBlueprintAsyncActionBase interface
	virtual void Activate() override;
	//~UBlueprintAsyncActionBase interface

	UFUNCTION(BlueprintCallable, category = Category)
		void Complete(FStableDiffusionPipelineImageResult& Result, TArray<UImagePipelineStageAsset*> Pipeline);

	UPROPERTY(BlueprintAssignable, BlueprintReadWrite)
		FImageStageResult OnStageCompleted;

	UPROPERTY(BlueprintAssignable, BlueprintReadWrite)
		FImageStageResult OnAllStagesCompleted;

private:
	UPROPERTY(Transient)
	TArray<UImagePipelineStageAsset*> Stages;

	UPROPERTY(Transient)
	FStableDiffusionInput Input;

	EInputImageSource ImageSourceType;
	bool AllowNSFW;
	int32 SeamlessMode;
	bool SaveLayers;
	UStableDiffusionImageResultAsset* OutAsset;
};
