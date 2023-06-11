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


UCLASS(meta = (DisplayName = "Layer options"))
class STABLEDIFFUSIONTOOLS_API UStencilLayerOptions : public ULayerProcessorOptions
{
	GENERATED_BODY()
public:

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Layer options")
		FActorLayer ActorLayer;
};


UCLASS(meta = (DisplayName = "Actor stencil layer"))
class STABLEDIFFUSIONTOOLS_API UStencilLayerProcessor: public ULayerProcessorBase
{
	GENERATED_BODY()

public:
	static FString StencilLayerMaterialAsset;

	virtual ULayerProcessorOptions* AllocateLayerOptions_Implementation() override;
	virtual void BeginCaptureLayer_Implementation(FIntPoint Size, USceneCaptureComponent2D* CaptureSource = nullptr, UObject* LayerOptions = nullptr) override;
	virtual UTextureRenderTarget2D* CaptureLayer(USceneCaptureComponent2D* CaptureSource, bool SingleFrame = true, UObject* LayerOptions = nullptr) override;
	virtual void EndCaptureLayer_Implementation(USceneCaptureComponent2D* CaptureSource = nullptr) override;

	FActorLayerStencilState ActorLayerState;

private:
	UPROPERTY(Transient)
	UMaterialInstanceDynamic* StencilMatInst;

	bool LastBloomState;
};
