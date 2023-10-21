#include "StableDiffusionImageResult.h"

void UStableDiffusionImageResultAsset::GetLastValidStageResult(FStableDiffusionPipelineImageResult& OutStageResult)
{
	for (int i = StageResults.Num() - 1; i >= 0; i--)
	{
		if (StageResults[i].ImageOutput.Completed && (IsValid(StageResults[i].PipelineStage)) ? !StageResults[i].PipelineStage->bSkipMainOutput : false)
		{
			OutStageResult = StageResults[i];
			break;
		}
	}
}
