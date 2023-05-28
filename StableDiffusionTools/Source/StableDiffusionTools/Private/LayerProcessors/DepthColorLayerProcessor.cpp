#include "LayerProcessors/DepthLayerProcessor.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Materials/MaterialInstanceDynamic.h"

UDepthLayerProcessor::UDepthLayerProcessor()
{
	CaptureBitDepth = SixteenBit;
}

void UDepthLayerProcessor::BeginCaptureLayer_Implementation(FIntPoint Size, USceneCaptureComponent2D* CaptureSource) {
	if (!CaptureSource)
		return;

	if (!DepthMatInst) {
		DepthMatInst = UMaterialInstanceDynamic::Create(PostMaterial, this);
	}
	ActivePostMaterialInstance = DepthMatInst;

	Super::BeginCaptureLayer_Implementation(Size, CaptureSource);
}


UTextureRenderTarget2D* UDepthLayerProcessor::CaptureLayer(USceneCaptureComponent2D* CaptureSource, bool SingleFrame)
{
	if (!DepthMatInst) {
		UE_LOG(LogTemp, Error, TEXT("No dynamic material instance set for %s instance"), *this->StaticClass()->GetDisplayNameText().ToString());
		return nullptr;
	}

	DepthMatInst->SetScalarParameterValue("DepthScale", SceneDepthScale);
	DepthMatInst->SetScalarParameterValue("StartDepth", SceneDepthOffset);

	// Capture the layer
	ULayerProcessorBase::CaptureLayer(CaptureSource, SingleFrame);

	return RenderTarget;
}

void UDepthLayerProcessor::EndCaptureLayer_Implementation(USceneCaptureComponent2D* CaptureSource) {
	if (!CaptureSource)
		return;

	Super::EndCaptureLayer_Implementation(CaptureSource);
}


TArray<FColor> UDepthLayerProcessor::ProcessLayer(UTextureRenderTarget2D* Layer)
{
	check(Layer);

	auto LinearPixels = ProcessLinearLayer(Layer);

	TArray<FColor> DepthPixels;
	DepthPixels.AddUninitialized(Layer->SizeX * Layer->SizeY);
	for (size_t idx = 0; idx < LinearPixels.Num(); ++idx) {
		int NormalizedDepth = LinearPixels[idx].R * 255;
		DepthPixels[idx] = FColor(NormalizedDepth, NormalizedDepth, NormalizedDepth, 255);
	}

	return DepthPixels;
}
