// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "StableDiffusionPromptMovieSceneTrack.h"
#include "StableDiffusionPromptMovieSceneSection.h"
#include "StableDiffusionOptionsTrack.h"
#include "StableDiffusionOptionsSection.h"
#include "StableDiffusionBridge.h"
#include "MoviePipelineDeferredPasses.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "StableDiffusionLayerProcessorTrack.h"
#include "StableDiffusionMoviePipeline.generated.h"


struct FStableDiffusionDeferredPassRenderStatePayload : public UMoviePipelineImagePassBase::IViewCalcPayload
{
	int32 CameraIndex;
	FIntPoint TileIndex; // Will always be 1,1 if no history-per-tile is enabled
	int32 SceneViewIndex;
};


/**
 * 
 */
UCLASS()
class STABLEDIFFUSIONSEQUENCER_API UStableDiffusionMoviePipeline : public UMoviePipelineDeferredPassBase
{
	GENERATED_BODY()

public:
	UStableDiffusionMoviePipeline();
	virtual void PostInitProperties();
	virtual void SetupForPipelineImpl(UMoviePipeline* InPipeline) override;
	virtual void TeardownForPipelineImpl(UMoviePipeline* InPipeline) override;
	void SetupImpl(const MoviePipeline::FMoviePipelineRenderPassInitSettings& InPassInitSettings) override;
	virtual void GatherOutputPassesImpl(TArray<FMoviePipelinePassIdentifier>& ExpectedRenderPasses) override;

#if WITH_EDITOR
	virtual FText GetDisplayText() const override { return NSLOCTEXT("MovieRenderPipeline", "DeferredBasePassSetting_DisplayName_StableDiffusion", "Stable Diffusion"); }
	virtual FText GetFooterText(UMoviePipelineExecutorJob* InJob) const override;
#endif
	virtual void GetViewShowFlags(FEngineShowFlags& OutShowFlag, EViewModeIndex& OutViewModeIndex) const override
	{
		OutShowFlag = FEngineShowFlags(EShowFlagInitMode::ESFIM_Game);
		//OutShowFlag.SetPathTracing(true);
		OutViewModeIndex = EViewModeIndex::VMI_Lit;
	}
	virtual int32 GetOutputFileSortingOrder() const override { return 2; }

	virtual bool IsAntiAliasingSupported() const { return false; }

	/**
	* If true, each generated frame will be upscaled.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "StableDiffusion|Outputs")
	bool bUpscale;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "StableDiffusion|Outputs")
	TSubclassOf<UStableDiffusionBridge> ImageGeneratorOverride;

	/**
	* The prefix to add to each upscaled frame. If empty, the upscaled frame will overwrite the source frame.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "StableDiffusion|Outputs")
	FString UpscaledFramePrefix = "UP_";

	/**
	* If you encounter false positive NSFW black frames in your animation, then enabling this option may help
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "StableDiffusion|Outputs")
	EPaddingMode PaddingMode;

	/**
	* Disable NSFW filter
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "StableDiffusion|Outputs")
	bool AllowNSFW;

	/**
	* Debug generated images from python
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "StableDiffusion|Outputs")
	bool DebugPythonImages = false;


protected:
	virtual void RenderSample_GameThreadImpl(const FMoviePipelineRenderPassMetrics& InSampleState) override;

	FSceneView* BeginSDLayerPass(FMoviePipelineRenderPassMetrics& InOutSampleState, TSharedPtr<FSceneViewFamilyContext>& ViewFamily);

private:
	virtual void BeginExportImpl() override;

	//TWeakObjectPtr<UTextureRenderTarget2D> StencilActorLayerRenderTarget;
	//FMoviePipelinePassIdentifier StencilPassIdentifier;
	UStableDiffusionOptionsTrack* OptionsTrack;
	TArray<UStableDiffusionPromptMovieSceneTrack*> PromptTracks;
	TArray<UStableDiffusionLayerProcessorTrack*> LayerProcessorTracks;

	void ApplyLayerOptions(TArray<FLayerProcessorContext>& Layers, size_t StageIndex, FFrameTime FrameTime);
};
