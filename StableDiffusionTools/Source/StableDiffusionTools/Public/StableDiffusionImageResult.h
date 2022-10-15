#pragma once

#include "CoreMinimal.h"
#include "StableDIffusionImageResult.generated.h"

USTRUCT(BlueprintType)
struct STABLEDIFFUSIONTOOLS_API FStableDiffusionImageResult
{
    GENERATED_BODY()
public:
    UPROPERTY(BlueprintReadWrite)
    FString Prompt;

    UPROPERTY(BlueprintReadWrite)
    int32 Seed;

    UPROPERTY(BlueprintReadWrite)
    FIntPoint StartImageSize;

    UPROPERTY(BlueprintReadWrite)
    FIntPoint GeneratedImageSize;

    UPROPERTY(BlueprintReadWrite)
    TArray<FColor> PixelData;

    UPROPERTY(BlueprintReadWrite)
    UTexture2D* GeneratedTexture;

    UPROPERTY(BlueprintReadWrite)
    UTexture2D* InputTexture;

    UPROPERTY(BlueprintReadWrite)
    UTexture2D* MaskTexture;
};
