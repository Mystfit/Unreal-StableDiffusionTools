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

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FImageStageResult, FStableDiffusionImageResult, StageResult);


UCLASS(Blueprintable)
class STABLEDIFFUSIONTOOLS_API UImagePipelineRunner : public UBlueprintAsyncActionBase
{
	GENERATED_UCLASS_BODY()
public:

	UFUNCTION(BlueprintCallable, meta = (Category = "Async", BlueprintInternalUseOnly = "true"))
		static UImagePipelineRunner* RunImagePipeline(TArray<UImagePipelineStageAsset*> Stages, FStableDiffusionInput Input, EInputImageSource ImageSourceType, bool AllowNSFW, EPaddingMode PaddingMode);

	// UBlueprintAsyncActionBase interface
	virtual void Activate() override;
	//~UBlueprintAsyncActionBase interface

	UFUNCTION(BlueprintCallable, category = Category)
		void Complete(FStableDiffusionImageResult& Result);

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
	EPaddingMode PaddingMode;
};
