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

    UPROPERTY(BlueprintReadWrite, Category = "StableDiffusion|Outputs")
    TArray<FColor> PixelData;
    
    UPROPERTY(BlueprintReadWrite, Category = "StableDiffusion|Outputs")
    int32 OutWidth;
    
    UPROPERTY(BlueprintReadWrite, Category = "StableDiffusion|Outputs")
    int32 OutHeight;

    UPROPERTY(BlueprintReadWrite, Category = "StableDiffusion|Outputs")
    UTexture2D* GeneratedTexture;

    UPROPERTY(BlueprintReadWrite, Category = "StableDiffusion|Outputs")
    bool Upsampled = false;
};
