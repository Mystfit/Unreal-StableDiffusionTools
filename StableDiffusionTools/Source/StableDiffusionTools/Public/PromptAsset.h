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
struct STABLEDIFFUSIONTOOLS_API FPromptTokenReplacements
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tokens")
		TArray<FString> Tokens;
};


USTRUCT(BlueprintType)
struct STABLEDIFFUSIONTOOLS_API FPrompt
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Prompts")
	FString Prompt;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Prompts")
	TEnumAsByte<EPromptSentiment> Sentiment;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Prompts")
	float Weight = 1.0f;

	UPROPERTY(BlueprintReadWrite, Category = "Prompts")
	TArray<FPromptTokenReplacements> TokenReplacements;
};



UCLASS()
class STABLEDIFFUSIONTOOLS_API UPromptAsset : public UDataAsset
{
	GENERATED_BODY()
public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Prompts")
	TArray<FPrompt> Prompts;
};
