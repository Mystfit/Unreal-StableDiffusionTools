#pragma once

#include "CoreMinimal.h"
#include "StableDiffusionGenerationOptions.h"
#include "StableDIffusionImageResult.generated.h"

USTRUCT(BlueprintType)
struct STABLEDIFFUSIONTOOLS_API FStableDiffusionImageResult
{
    GENERATED_BODY()
public:
    UPROPERTY(BlueprintReadWrite, Category = "Inputs")
    FStableDiffusionInput Input;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Inputs")
    FStableDiffusionModelOptions Model;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Inputs")
    FStableDiffusionPipelineOptions Pipeline;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Inputs")
    FStableDiffusionModelOptions LORA;

    UPROPERTY(BlueprintReadWrite, Category = "Outputs")
    TEnumAsByte<EImageType> OutputType = EImageType::Image;

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


UCLASS(BlueprintType)
class STABLEDIFFUSIONTOOLS_API UStableDiffusionImageResultAsset : public UPrimaryDataAsset
{
    GENERATED_BODY()
public:
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Inputs")
    FStableDiffusionGenerationOptions ImageInputs;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Outputs")
    FStableDiffusionImageResult ImageOutput;
};
