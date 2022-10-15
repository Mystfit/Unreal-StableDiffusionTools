// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "StableDiffusionPromptMovieSceneTrack.h"
#include "StableDiffusionPromptMovieSceneSection.h"
#include "StableDiffusionOptionsTrack.h"
#include "StableDiffusionOptionsSection.h"
#include "MoviePipelineDeferredPasses.h"
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
	}
	virtual void SetupForPipelineImpl(UMoviePipeline* InPipeline);

#if WITH_EDITOR
	virtual FText GetDisplayText() const override { return NSLOCTEXT("MovieRenderPipeline", "DeferredBasePassSetting_DisplayName_StableDiffusion", "Stable Diffusion"); }
	virtual FText GetFooterText(UMoviePipelineExecutorJob* InJob) const override;
#endif
	virtual void GetViewShowFlags(FEngineShowFlags& OutShowFlag, EViewModeIndex& OutViewModeIndex) const override
	{
		OutShowFlag = FEngineShowFlags(EShowFlagInitMode::ESFIM_Game);
		OutShowFlag.SetPathTracing(true);
		OutViewModeIndex = EViewModeIndex::VMI_PathTracing;
	}
	virtual int32 GetOutputFileSortingOrder() const override { return 2; }

	virtual bool IsAntiAliasingSupported() const { return false; }

protected:
	virtual void RenderSample_GameThreadImpl(const FMoviePipelineRenderPassMetrics& InSampleState) override;

private:
	UStableDiffusionOptionsTrack* OptionsTrack;
	TArray<UStableDiffusionPromptMovieSceneTrack*> PromptTracks;
};
