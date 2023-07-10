// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Engine/SceneCapture2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "IDetailCustomization.h"
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
	latent UMETA(DisplayName = "Latent"),
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
	void BeginCaptureLayer(UWorld* World, FIntPoint Size, USceneCaptureComponent2D* CaptureSource = nullptr, UObject* LayerOptions = nullptr);

	virtual UTextureRenderTarget2D* CaptureLayer(USceneCaptureComponent2D* CaptureSource, bool SingleFrame = true, UObject* LayerOptions = nullptr);
	
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Layer processor")
	void EndCaptureLayer(UWorld* World, USceneCaptureComponent2D* CaptureSource = nullptr);

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
	UMaterialInterface* PostMaterial;

	UFUNCTION(BlueprintCallable, Category = "Stable Diffusion|Layer source")
	UMaterialInterface* GetActivePostMaterial();

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Stable Diffusion|Layer source")
	TEnumAsByte<ELayerImageType> DefaultLayerType;

	static const TMap<ELayerImageType, FString> ReverseLayerImageTypeLookup;
	static const TMap<FString, ELayerImageType> LayerImageTypeLookup;

	FPrimaryAssetId GetPrimaryAssetId() const override;

protected:
	UTextureRenderTarget2D* GetOrAllocateRenderTarget(FIntPoint Size);
	
	void SetActivePostMaterial(TObjectPtr<UMaterialInterface> Material);

private:
	UTextureRenderTarget2D* RenderTarget;

	UMaterialInterface* ActivePostMaterialInstance;
}; 


UCLASS(Blueprintable, EditInlineNew, CollapseCategories) //CollapseCategories , hidecategories = UObject
class STABLEDIFFUSIONTOOLS_API ULayerProcessorOptions : public UObject
{
	GENERATED_BODY()
public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Layer options")
	float Strength = 1.0f;
};

USTRUCT(BlueprintType)
struct STABLEDIFFUSIONTOOLS_API FLayerProcessorContext
{
	GENERATED_USTRUCT_BODY()
public:
	//FLayerProcessorContext() {}
	//FLayerProcessorContext(const FLayerProcessorContext& Source);
	//FLayerProcessorContext(FLayerProcessorContext&& Source);


	UPROPERTY(BlueprintReadWrite, Transient)
		TArray<FColor> LayerPixels;

	UPROPERTY(BlueprintReadWrite, Transient)
		TArray<uint8> LatentData;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta=(Category = "Layers"))
		TObjectPtr<ULayerProcessorBase> Processor = nullptr;

	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Instanced, meta = (Category = "Layers", EditCondition = "Processor != nullptr", EditConditionHides))
		TObjectPtr<ULayerProcessorOptions> ProcessorOptions = nullptr;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (UsesHierarchy = true, Category = "Layers"))
		TEnumAsByte<ELayerImageType> LayerType = unknown;

	/*
	* Identifying role key for this layer that will be used to assign it to the correct pipe keyword argument
	*/
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (Category = "Layers", EditCondition = "LayerType == ELayerImageType::custom", EditConditionHides))
		FString Role = "image";
};


//class ULayerProcessorOptionsDetails : public IPropertyTypeCustomization
//{
//public:
//	/** Makes a new instance of this detail layout class for a specific detail view requesting it */
//	static TSharedRef<IPropertyTypeCustomization> MakeInstance();
//
//	/** IDetailCustomization interface */
//	virtual void CustomizeHeader(TSharedRef<IPropertyHandle> PropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& CustomizationUtils) override;
//
//	/**
//	 * Called when the children of the property should be customized or extra rows added
//	 *
//	 * @param PropertyHandle			Handle to the property being customized
//	 * @param StructBuilder				A builder for adding children
//	 * @param StructCustomizationUtils	Utilities for customization
//	 */
//	virtual void CustomizeChildren(TSharedRef<IPropertyHandle> PropertyHandle, IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& CustomizationUtils) override;
//
//private:
//	TSharedPtr<IPropertyHandle> StructPropertyHandle;
//};
