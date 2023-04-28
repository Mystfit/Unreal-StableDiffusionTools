#pragma once

#include "CoreMinimal.h"
#include "LayerProcessorBase.h"
#include "ActorLayerUtilities.h"
#include "StableDiffusionBlueprintLibrary.h"
#include "StencilLayerProcessor.generated.h"

class STABLEDIFFUSIONTOOLS_API FActorLayerStencilState {
public:
	void CaptureActorLayer(const FActorLayer& Layer);
	void RestoreActorLayer();
private:
	// Stencil values
	TMap<UPrimitiveComponent*, FStencilValues> ActorLayerSavedStencilValues;

	// Cache the custom stencil value.
	TOptional<int32> PreviousCustomDepthValue;
};

struct STABLEDIFFUSIONTOOLS_API FScopedActorLayerStencil {
public:
	FScopedActorLayerStencil() = delete;
	FScopedActorLayerStencil(const FScopedActorLayerStencil& ref);
	FScopedActorLayerStencil(const FActorLayer& Layer, bool RestoreOnDelete = true);
	~FScopedActorLayerStencil();

private:
	FActorLayerStencilState State;
	bool RestoreOnDelete;
};

UCLASS(meta = (DisplayName = "Actor stencil layer"))
class STABLEDIFFUSIONTOOLS_API UStencilLayerProcessor: public ULayerProcessorBase
{
	GENERATED_BODY()

public:
	static FString StencilLayerMaterialAsset;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Layer options")
	FActorLayer ActorLayer;

	virtual void BeginCaptureLayer_Implementation(FIntPoint Size, USceneCaptureComponent2D* CaptureSource = nullptr) override;

	virtual UTextureRenderTarget2D* CaptureLayer(USceneCaptureComponent2D* CaptureSource, bool SingleFrame = true) override;

	virtual void EndCaptureLayer_Implementation(USceneCaptureComponent2D* CaptureSource = nullptr) override;

	FActorLayerStencilState ActorLayerState;

private:
	UPROPERTY(Transient)
	UMaterialInstanceDynamic* StencilMatInst;

	bool LastBloomState;
};
