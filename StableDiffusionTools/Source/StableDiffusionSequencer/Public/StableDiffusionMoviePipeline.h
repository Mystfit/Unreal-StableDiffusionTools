// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "StableDiffusionPromptMovieSceneTrack.h"
#include "StableDiffusionPromptMovieSceneSection.h"
#include "StableDiffusionOptionsTrack.h"
#include "StableDiffusionOptionsSection.h"
#include "MoviePipelineDeferredPasses.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "StableDiffusionMoviePipeline.generated.h"



/**
 * 
 */
UCLASS()
class STABLEDIFFUSIONSEQUENCER_API UStableDiffusionMoviePipeline : public UMoviePipelineDeferredPassBase
{
	GENERATED_BODY()

public:
	UStableDiffusionMoviePipeline() : UMoviePipelineDeferredPassBase()
	{
		PassIdentifier = FMoviePipelinePassIdentifier("StableDiffusion");
		StencilPassIdentifier = FMoviePipelinePassIdentifier("StableDiffusion_StencilPass");
	}
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

	/**
	* The prefix to add to each upscaled frame. If empty, the upscaled frame will overwrite the source frame.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "StableDiffusion|Outputs")
	FString UpscaledFramePrefix;


protected:
	virtual void RenderSample_GameThreadImpl(const FMoviePipelineRenderPassMetrics& InSampleState) override;

private:
	virtual void BeginExportImpl() override;

	TObjectPtr<UMaterialInterface> StencilMatInst;
	TObjectPtr<UTextureRenderTarget2D> StencilActorLayerRenderTarget;
	FMoviePipelinePassIdentifier StencilPassIdentifier;
	UStableDiffusionOptionsTrack* OptionsTrack;
	TArray<UStableDiffusionPromptMovieSceneTrack*> PromptTracks;
};
