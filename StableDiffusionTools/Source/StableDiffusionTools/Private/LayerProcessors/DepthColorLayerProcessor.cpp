#include "LayerProcessors/DepthLayerProcessor.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Materials/MaterialInstanceDynamic.h"

UDepthLayerProcessor::UDepthLayerProcessor()
{
	CaptureBitDepth = SixteenBit;
}

ULayerProcessorOptions* UDepthLayerProcessor::AllocateLayerOptions_Implementation()
{
	return NewObject<UDepthLayerOptions>();
}

void UDepthLayerProcessor::BeginCaptureLayer_Implementation(UWorld* World, FIntPoint Size, USceneCaptureComponent2D* CaptureSource, UObject* LayerOptions) {
	if (!DepthMatInst) {
		DepthMatInst = UMaterialInstanceDynamic::Create(PostMaterial, this);
	}

	if (auto DepthOptions = Cast<UDepthLayerOptions>(LayerOptions)) {
		DepthMatInst->SetScalarParameterValue("DepthScale", DepthOptions->SceneDepthScale);
		DepthMatInst->SetScalarParameterValue("StartDepth", DepthOptions->SceneDepthOffset);
	}

	SetActivePostMaterial(DepthMatInst);
	Super::BeginCaptureLayer_Implementation(World, Size, CaptureSource, LayerOptions);
}


UTextureRenderTarget2D* UDepthLayerProcessor::CaptureLayer(USceneCaptureComponent2D* CaptureSource, bool SingleFrame, UObject* LayerOptions)
{
	if (!DepthMatInst) {
		UE_LOG(LogTemp, Error, TEXT("No dynamic material instance set for %s instance"), *this->StaticClass()->GetDisplayNameText().ToString());
		return nullptr;
	}

	// Capture the layer
	return ULayerProcessorBase::CaptureLayer(CaptureSource, SingleFrame);
}

void UDepthLayerProcessor::EndCaptureLayer_Implementation(UWorld* World, USceneCaptureComponent2D* CaptureSource) {
	Super::EndCaptureLayer_Implementation(World, CaptureSource);
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
