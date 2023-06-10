#pragma once

#include "CoreMinimal.h"
#include "LayerProcessorBase.h"
#include "DepthLayerProcessor.generated.h"

UCLASS(meta=(DisplayName="Depth layer"))
class STABLEDIFFUSIONTOOLS_API UDepthLayerProcessor : public ULayerProcessorBase
{
	GENERATED_BODY()

public:
	UDepthLayerProcessor();

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Layer options")
	float SceneDepthScale = 2000.0f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Layer options")
	float SceneDepthOffset = 0.0f;

	virtual void BeginCaptureLayer_Implementation(FIntPoint Size, USceneCaptureComponent2D* CaptureSource = nullptr) override;
	virtual UTextureRenderTarget2D* CaptureLayer(USceneCaptureComponent2D* CaptureSource, bool SingleFrame = true) override;
	virtual void EndCaptureLayer_Implementation(USceneCaptureComponent2D* CaptureSource = nullptr) override;
	virtual TArray<FColor> ProcessLayer(UTextureRenderTarget2D* Layer) override;

private:
	UPROPERTY(Transient)
	UMaterialInstanceDynamic* DepthMatInst;

	UPROPERTY(Transient)
	UMaterialInterface* OriginalMaterial;
};
