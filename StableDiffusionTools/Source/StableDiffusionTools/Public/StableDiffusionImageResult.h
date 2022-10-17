#pragma once

#include "CoreMinimal.h"
#include "StableDiffusionGenerationOptions.h"
#include "StableDIffusionImageResult.generated.h"

USTRUCT(BlueprintType)
struct STABLEDIFFUSIONTOOLS_API FStableDiffusionImageResult
{
    GENERATED_BODY()
public:
    UPROPERTY(BlueprintReadWrite)
    FStableDiffusionInput Input;

    UPROPERTY(BlueprintReadWrite)
    TArray<FColor> PixelData;

    UPROPERTY(BlueprintReadWrite)
    UTexture2D* GeneratedTexture;
};
