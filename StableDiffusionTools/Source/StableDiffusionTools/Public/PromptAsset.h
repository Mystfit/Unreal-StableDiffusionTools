#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "PromptAsset.generated.h"

/**
 *
 */

UENUM(BlueprintType)
enum EPromptSentiment
{
	Positive	UMETA(DisplayName = "Positive"),
	Negative	UMETA(DisplayName = "Negative"),
};


USTRUCT(BlueprintType)
struct STABLEDIFFUSIONTOOLS_API FPrompt
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stable Diffusion|Prompts")
	FString Prompt;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stable Diffusion|Prompts")
	TEnumAsByte<EPromptSentiment> Sentiment;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stable Diffusion|Prompts")
	float Weight;
};



UCLASS()
class STABLEDIFFUSIONTOOLS_API UPromptAsset : public UDataAsset
{
	GENERATED_BODY()
public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TArray<FPrompt> Prompts;
};
