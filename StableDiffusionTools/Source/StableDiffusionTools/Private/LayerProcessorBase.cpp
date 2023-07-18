// Fill out your copyright notice in the Description page of Project Settings.

#include "LayerProcessorBase.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Materials/MaterialInstanceDynamic.h"

const TMap<ELayerImageType, FString> ULayerProcessorBase::ReverseLayerImageTypeLookup = {
	{ELayerImageType::unknown, "unknown"},
	{ELayerImageType::image, "image"},
	{ELayerImageType::control_image, "control_image"},
	{ELayerImageType::custom, "custom"}
};

const TMap<FString, ELayerImageType> ULayerProcessorBase::LayerImageTypeLookup = {
	{"unknown", ELayerImageType::unknown},
	{"image", ELayerImageType::image},
	{"control_image", ELayerImageType::control_image},
	{"custom", ELayerImageType::custom}
};


void ULayerProcessorBase::BeginCaptureLayer_Implementation(UWorld* World, FIntPoint Size, USceneCaptureComponent2D* CaptureSource, UObject* LayerOptions)
{
	if (!ActivePostMaterialInstance)
		ActivePostMaterialInstance = UMaterialInstanceDynamic::Create(PostMaterial, this);

	if (CaptureSource)
	{
		// Create render target to hold our scene capture data
		CaptureSource->TextureTarget = GetOrAllocateRenderTarget(Size);

		//// Remember original properties we're overriding in the capture component
		//auto LastCaptureEveryFrame = CaptureSource->bCaptureEveryFrame;
		//auto LastCaptureOnMovement = CaptureSource->bCaptureOnMovement;
		//auto LastCompositeMode = CaptureSource->CompositeMode;
		//auto LastCaptureSource = CaptureSource->CaptureSource;
		//auto LastTextureTarget = CaptureSource->TextureTarget;
		//auto LastBloom = CaptureSource->ShowFlags.Bloom;
		//auto LastBlendables = CaptureSource->PostProcessSettings.WeightedBlendables;
		//auto LastAlwaysPersist = CaptureSource->bAlwaysPersistRenderingState;

		// Set capture overrides
		CaptureSource->bCaptureEveryFrame = true;
		CaptureSource->bCaptureOnMovement = false;
		CaptureSource->CompositeMode = SCCM_Overwrite;
		CaptureSource->bAlwaysPersistRenderingState = true;
		CaptureSource->CaptureSource = SourceType;

		if (ActivePostMaterialInstance)
			CaptureSource->AddOrUpdateBlendable(ActivePostMaterialInstance);
	}
}

ULayerProcessorOptions* ULayerProcessorBase::AllocateLayerOptions_Implementation()
{
	return NewObject<ULayerProcessorOptions>();
}

UTextureRenderTarget2D* ULayerProcessorBase::CaptureLayer(USceneCaptureComponent2D* CaptureSource, bool SingleFrame, UObject* LayerOptions)
{
	UTextureRenderTarget2D* result = nullptr;
	if (CaptureSource) {
		// Capture scene and get main render pass
		if (SingleFrame) {
			CaptureSource->CaptureScene();
		}
		result = CaptureSource->TextureTarget;
		return result;
	}

	return RenderTarget;
}

void ULayerProcessorBase::EndCaptureLayer_Implementation(UWorld* World, USceneCaptureComponent2D* CaptureSource)
{
	ActivePostMaterialInstance = nullptr;

	if (CaptureSource) {
		//// Restore original capture component properties
		//CaptureSource->bCaptureEveryFrame = LastCaptureEveryFrame;
		//CaptureSource->bCaptureOnMovement = LastCaptureOnMovement;
		//CaptureSource->CompositeMode = LastCompositeMode;
		//CaptureSource->CaptureSource = LastCaptureSource;
		//CaptureSource->TextureTarget = LastTextureTarget;
		//CaptureSource->ShowFlags.Bloom = LastBloom;
		//CaptureSource->PostProcessSettings.WeightedBlendables = LastBlendables;
		//CaptureSource->bAlwaysPersistRenderingState = LastAlwaysPersist;

		//Cleanup
		CaptureSource->RemoveBlendable(ActivePostMaterialInstance);
	}
}

TArray<FColor> ULayerProcessorBase::ProcessLayer(UTextureRenderTarget2D* Layer)
{
	TArray<FColor> FinalColor;

	if (IsValid(Layer)) {
		// Create destination pixel arrays
		FinalColor.AddUninitialized(Layer->SizeX * Layer->SizeY);
		FTextureRenderTargetResource* FullFrameRT_TexRes = Layer->GameThread_GetRenderTargetResource();
		FullFrameRT_TexRes->ReadPixels(FinalColor, FReadSurfaceDataFlags());
	}
	return MoveTemp(FinalColor);

}

TArray<FLinearColor> ULayerProcessorBase::ProcessLinearLayer(UTextureRenderTarget2D* Layer)
{
	TArray<FLinearColor> FinalColor;
	if (IsValid(Layer)) {
		// Create destination pixel arrays
		FinalColor.AddUninitialized(Layer->SizeX * Layer->SizeY);
		FTextureRenderTargetResource* FullFrameRT_TexRes = Layer->GameThread_GetRenderTargetResource();
		FullFrameRT_TexRes->ReadLinearColorPixels(FinalColor, FReadSurfaceDataFlags());
	}
	return MoveTemp(FinalColor);
}

UMaterialInterface* ULayerProcessorBase::GetActivePostMaterial()
{
	return ActivePostMaterialInstance;
}

UTextureRenderTarget2D* ULayerProcessorBase::GetOrAllocateRenderTarget(FIntPoint Size)
{
	if (!RenderTarget->IsValidLowLevel() || RenderTarget->SizeX != Size.X || RenderTarget->SizeY != Size.Y) {
		RenderTarget = NewObject<UTextureRenderTarget2D>(this);
		RenderTarget->InitCustomFormat(Size.X, Size.Y, (CaptureBitDepth == EightBit) ? PF_R8G8B8A8 : PF_FloatRGBA, false);
		RenderTarget->UpdateResourceImmediate(true);
	}	
	check(RenderTarget);
	return RenderTarget;
}

void ULayerProcessorBase::SetActivePostMaterial(TObjectPtr<UMaterialInterface> Material)
{
	ActivePostMaterialInstance = Material;
}

FPrimaryAssetId ULayerProcessorBase::GetPrimaryAssetId() const
{
	auto PrimaryId = Super::GetPrimaryAssetId();
	return PrimaryId;
	//// Check if the asset is a blueprint class
	//if (GetClass()->ClassGeneratedBy != nullptr)
	//{
	//	// Get the blueprint class
	//	UClass* BlueprintClass = Cast<UClass>(GetClass()->ClassGeneratedBy);
	//	check(BlueprintClass != nullptr);

	//	// Get the primary asset id of the blueprint class
	//	const FString BlueprintClassName = BlueprintClass->GetName();
	//	const FString AssetRegistryName = GetPathName();
	//	const FPrimaryAssetType AssetType = ULayerProcessorBase::StaticClass()->GetFName();
	//	const FName PrimaryAssetName(*BlueprintClassName);

	//	return FPrimaryAssetId(AssetType, PrimaryAssetName);
	//}

	// This asset is not a blueprint, return its own primary asset id
	//return FPrimaryAssetId(ULayerProcessorBase::StaticClass()->GetFName(), FName(GetPathName()));
}
