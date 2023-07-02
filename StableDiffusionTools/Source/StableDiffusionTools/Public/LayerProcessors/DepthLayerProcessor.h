#pragma once

#include "CoreMinimal.h"
#include "LayerProcessorBase.h"
#include "DepthLayerProcessor.generated.h"

UCLASS(EditInlineNew, Blueprintable)
class STABLEDIFFUSIONTOOLS_API UDepthLayerOptions : public ULayerProcessorOptions
{
	GENERATED_BODY()
public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Layer options")
		float SceneDepthScale = 2000.0f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Layer options")
		float SceneDepthOffset = 0.0f;
};


UCLASS()
class STABLEDIFFUSIONTOOLS_API UDepthLayerProcessor : public ULayerProcessorBase
{
	GENERATED_BODY()

public:
	UDepthLayerProcessor();

	virtual ULayerProcessorOptions* AllocateLayerOptions_Implementation() override;
	virtual void BeginCaptureLayer_Implementation(UWorld* World, FIntPoint Size, USceneCaptureComponent2D* CaptureSource = nullptr, UObject* LayerOptions = nullptr) override;
	virtual UTextureRenderTarget2D* CaptureLayer(USceneCaptureComponent2D* CaptureSource, bool SingleFrame = true, UObject* LayerOptions = nullptr) override;
	virtual void EndCaptureLayer_Implementation(UWorld* World, USceneCaptureComponent2D* CaptureSource = nullptr) override;
	virtual TArray<FColor> ProcessLayer(UTextureRenderTarget2D* Layer) override;

private:
	UPROPERTY(Transient)
	UMaterialInstanceDynamic* DepthMatInst;

	UPROPERTY(Transient)
	UMaterialInterface* OriginalMaterial;
};
