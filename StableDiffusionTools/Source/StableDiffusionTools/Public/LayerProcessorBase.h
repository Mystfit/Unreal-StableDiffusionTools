// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Engine/SceneCapture2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "LayerProcessorBase.generated.h"


UENUM()
enum ELayerBitDepth
{
	EightBit UMETA(DisplayName = "8 bit channel bit depth"),
	SixteenBit UMETA(DisplayName = "16 bit channel bit depth"),
	LayerBitDepth_MAX
};


UENUM()
enum ELayerImageType
{
	unknown UMETA(DisplayName = "Unknown", Hidden), 
	image UMETA(DisplayName = "Img2Img"),
	control_image UMETA(DisplayName = "ControlNet"),
	custom UMETA(DisplayName = "Custom"),
	ELayerImageType_MAX UMETA(Hidden)
};


/**
 * 
 */
UCLASS(Blueprintable, meta=(DisplayName = "Base layer processor"))
class STABLEDIFFUSIONTOOLS_API ULayerProcessorBase : public UPrimaryDataAsset
{
	GENERATED_BODY()
public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Stable Diffusion|Layer source")
	TEnumAsByte<ESceneCaptureSource> SourceType;

	/*
	* Script to run during model initialisation in case this processor needs to interact with the model.
	*/
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (MultiLine = "true", Category = "Stable Diffusion|Layer source"))
	FString PythonModelInitScript;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Layer processor")
	ULayerProcessorOptions* AllocateLayerOptions();

	/*
	* Script to run just the model generates a new image. Performs any python image transformations required.
	*/
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (MultiLine = "true", Category = "Stable Diffusion|Layer source"))
	FString PythonTransformScript;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Stable Diffusion|Layer source")
	TEnumAsByte<ELayerBitDepth> CaptureBitDepth = EightBit;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Layer processor")
	void BeginCaptureLayer(FIntPoint Size, USceneCaptureComponent2D* CaptureSource = nullptr, UObject* LayerOptions = nullptr);

	virtual UTextureRenderTarget2D* CaptureLayer(USceneCaptureComponent2D* CaptureSource, bool SingleFrame = true, UObject* LayerOptions = nullptr);
	
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Layer processor")
	void EndCaptureLayer(USceneCaptureComponent2D* CaptureSource = nullptr);

	/// <summary>
	/// Process a captured layer and convert to a pixel array
	/// </summary>
	/// <param name="Layer"></param>
	/// <returns></returns>
	virtual TArray<FColor> ProcessLayer(UTextureRenderTarget2D* Layer);

	/// <summary>
	/// Capture a linear texture from the provided capture source and process it
	/// </summary>
	/// <param name="CaptureSource"></param>
	/// <returns></returns>
	virtual TArray<FLinearColor> ProcessLinearLayer(UTextureRenderTarget2D* Layer);

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Stable Diffusion|Layer source")
	UTextureRenderTarget2D* RenderTarget;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Stable Diffusion|Layer source")
	UMaterialInterface* PostMaterial;

	UMaterialInterface* GetActivePostMaterial();

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Stable Diffusion|Layer source")
	TEnumAsByte<ELayerImageType> DefaultLayerType;

	

protected:
	UTextureRenderTarget2D* GetOrAllocateRenderTarget(FIntPoint Size);

	UMaterialInterface* ActivePostMaterialInstance;
}; 


UCLASS(Blueprintable, BlueprintType, meta = (DisplayName = "Layer options"))
class STABLEDIFFUSIONTOOLS_API ULayerProcessorOptions : public UObject
{
	GENERATED_BODY()
};


USTRUCT(BlueprintType)
struct STABLEDIFFUSIONTOOLS_API FLayerData
{
	GENERATED_BODY()
public:
	UPROPERTY(BlueprintReadWrite, Transient)
		TArray<FColor> LayerPixels;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta=(UsesHierarchy=true, Category = "Layers"))
		ULayerProcessorBase* Processor;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (UsesHierarchy = true, Category = "Layers"))
		ULayerProcessorOptions* ProcessorOptions;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (UsesHierarchy = true, Category = "Layers"))
		TEnumAsByte<ELayerImageType> LayerType;

	/*
	* Identifying role key for this layer that will be used to assign it to the correct pipe keyword argument
	*/
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (Category = "Layers", EditCondition = "LayerType == ELayerImageType::custom", EditConditionHides))
		FString Role = "image";
};
