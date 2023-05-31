#pragma once

#include "CoreMinimal.h"
#include "StableDiffusionGenerationOptions.h"
#include "StableDIffusionImageResult.generated.h"

USTRUCT(BlueprintType)
struct STABLEDIFFUSIONTOOLS_API FStableDiffusionImageResult
{
    GENERATED_BODY()
public:
    UPROPERTY(BlueprintReadWrite, Category = "StableDiffusion|Outputs")
    FStableDiffusionInput Input;

    //UPROPERTY(BlueprintReadWrite, Category = "StableDiffusion|Outputs")
    //TArray<FColor> PixelData;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "StableDiffusion|Outputs")
    UTexture2D* OutTexture;
    
    UPROPERTY(BlueprintReadWrite, Category = "StableDiffusion|Outputs")
    int32 OutWidth;
    
    UPROPERTY(BlueprintReadWrite, Category = "StableDiffusion|Outputs")
    int32 OutHeight;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "StableDiffusion|Outputs")
    bool Upsampled = false;

    UPROPERTY(BlueprintReadWrite, Category = "StableDiffusion|Outputs")
    bool Completed = false;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "StableDiffusion|Outputs")
    FMinimalViewInfo View;
};


UCLASS(BlueprintType)
class STABLEDIFFUSIONTOOLS_API UStableDiffusionImageResultAsset : public UPrimaryDataAsset
{
    GENERATED_BODY()
public:
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "StableDiffusion|Outputs")
    FStableDiffusionGenerationOptions ImageInputs;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "StableDiffusion|Outputs")
    FStableDiffusionImageResult ImageOutput;
};
