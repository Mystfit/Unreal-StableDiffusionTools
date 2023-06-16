// Fill out your copyright notice in the Description page of Project Settings.

#include "LayerProcessorBase.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Components/SceneCaptureComponent2D.h"


void ULayerProcessorBase::BeginCaptureLayer_Implementation(FIntPoint Size, USceneCaptureComponent2D* CaptureSource, UObject* LayerOptions)
{
	if (!CaptureSource)
		return;

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

	if (!ActivePostMaterialInstance)
		ActivePostMaterialInstance = PostMaterial;

	if (ActivePostMaterialInstance)
		CaptureSource->AddOrUpdateBlendable(ActivePostMaterialInstance);
}

ULayerProcessorOptions* ULayerProcessorBase::AllocateLayerOptions_Implementation()
{
	return nullptr;
}

UTextureRenderTarget2D* ULayerProcessorBase::CaptureLayer(USceneCaptureComponent2D* CaptureSource, bool SingleFrame, UObject* LayerOptions)
{
	if (!CaptureSource)
		return nullptr;

	// Capture scene and get main render pass
	if (SingleFrame){
		CaptureSource->CaptureScene();
	}

	return CaptureSource->TextureTarget;
}

void ULayerProcessorBase::EndCaptureLayer_Implementation(USceneCaptureComponent2D* CaptureSource)
{
	if (!CaptureSource)
		return;

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
	ActivePostMaterialInstance = nullptr;
}

TArray<FColor> ULayerProcessorBase::ProcessLayer(UTextureRenderTarget2D* Layer)
{
	if (!IsValid(Layer))
		return TArray<FColor>();
	
	// Create destination pixel arrays
	TArray<FColor> FinalColor;
	FinalColor.AddUninitialized(Layer->SizeX * Layer->SizeY);
	FTextureRenderTargetResource* FullFrameRT_TexRes = Layer->GameThread_GetRenderTargetResource();
	FullFrameRT_TexRes->ReadPixels(FinalColor, FReadSurfaceDataFlags());

	return MoveTemp(FinalColor);
}

TArray<FLinearColor> ULayerProcessorBase::ProcessLinearLayer(UTextureRenderTarget2D* Layer)
{
	if (!IsValid(Layer))
		return TArray<FLinearColor>();

	// Create destination pixel arrays
	TArray<FLinearColor> FinalColor;
	FinalColor.AddUninitialized(Layer->SizeX * Layer->SizeY);
	FTextureRenderTargetResource* FullFrameRT_TexRes = Layer->GameThread_GetRenderTargetResource();
	FullFrameRT_TexRes->ReadLinearColorPixels(FinalColor, FReadSurfaceDataFlags());

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
