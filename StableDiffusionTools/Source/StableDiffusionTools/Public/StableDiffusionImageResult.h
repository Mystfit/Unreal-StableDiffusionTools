#pragma once

#include "CoreMinimal.h"
#include "StableDiffusionGenerationOptions.h"
#include "StableDIffusionImageResult.generated.h"

USTRUCT(BlueprintType)
struct STABLEDIFFUSIONTOOLS_API FStableDiffusionImageResult
{
    GENERATED_BODY()
public:
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Outputs")
    UTexture2D* OutTexture = nullptr;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Outputs")
    TArray<uint8> OutLatent;
    
    UPROPERTY(BlueprintReadWrite, Category = "Outputs")
    int32 OutWidth = 0;
    
    UPROPERTY(BlueprintReadWrite, Category = "Outputs")
    int32 OutHeight = 0;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Outputs")
    bool Upsampled = false;

    UPROPERTY(BlueprintReadWrite, Category = "Outputs")
    bool Completed = false;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Outputs")
    FMinimalViewInfo View;
};


USTRUCT(BlueprintType)
struct STABLEDIFFUSIONTOOLS_API FStableDiffusionPipelineImageResult
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Instanced, Category = "Inputs")
        UImagePipelineStageAsset* PipelineStage = nullptr;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Outputs")
        FStableDiffusionImageResult ImageOutput;
};


UCLASS(BlueprintType)
class STABLEDIFFUSIONTOOLS_API UStableDiffusionImageResultAsset : public UPrimaryDataAsset
{
    GENERATED_BODY()
public:
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Inputs")
    FStableDiffusionInput GenerationInputs;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stages")
    TArray<FStableDiffusionPipelineImageResult> StageResults;

    UFUNCTION(BlueprintCallable, Category = "Output")
    void GetLastValidStageResult(FStableDiffusionPipelineImageResult& OutStageResult);
};
