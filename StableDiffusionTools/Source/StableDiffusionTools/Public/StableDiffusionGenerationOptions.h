#pragma once

#include "CoreMinimal.h"
#include "StableDiffusionGenerationOptions.generated.h"

USTRUCT(BlueprintType)
struct STABLEDIFFUSIONTOOLS_API FStableDiffusionGenerationOptions
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stable Diffusion|Generation")
	float Strength = 0.75;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stable Diffusion|Generation")
	int32 Iterations = 50;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stable Diffusion|Generation")
	int32 Seed = -1;
};
