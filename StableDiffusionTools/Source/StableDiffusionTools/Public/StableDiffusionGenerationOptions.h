#pragma once

#include "CoreMinimal.h"
#include "PromptAsset.h"
#include "ActorLayerUtilities.h"
#include "LayerProcessorBase.h"

#include "StableDiffusionGenerationOptions.generated.h"

UENUM(BlueprintType, meta = (Bitflags, UseEnumValuesAsMaskValuesInEditor = "true"))
enum class EPipelineCapabilities : uint8 {
	None = 0 UMETA(Hidden),
	INPAINT = 0x01,
	DEPTH = 0x02,
	STRENGTH = 0x04,
	CONTROL = 0x08
};
ENUM_CLASS_FLAGS(EPipelineCapabilities);

UENUM(BlueprintType)
enum class EInputImageSource : uint8 {
	Viewport UMETA(DisplayName = "Viewport"),
	SceneCapture2D UMETA(DisplayName = "Scene Capture Actor"),
	Texture UMETA(DisplayName = "Texture")
};
ENUM_CLASS_FLAGS(EInputImageSource);

UENUM(BlueprintType)
enum class EPaddingMode : uint8 {
	//'zeros', 'reflect', 'replicate' or 'circular'.
	zeros UMETA(DisplayName = "Zeroes"),
	reflect UMETA(DisplayName = "Reflect"),
	replicate UMETA(DisplayName = "Replicate"),
	circular UMETA(DisplayName = "Circular")
};
ENUM_CLASS_FLAGS(EPaddingMode);

UENUM(BlueprintType)
enum class EModelStatus : uint8 {
	Unloaded UMETA(DisplayName = "Unloaded"),
	Loading UMETA(DisplayName = "Loading"),
	Loaded UMETA(DisplayName = "Loaded"),
	Downloading UMETA(DisplayName = "Downloading"),
	Error UMETA(DisplayName = "Error")
};
ENUM_CLASS_FLAGS(EModelStatus);


USTRUCT(BlueprintType, meta=(UsesHierarchy=true))
struct STABLEDIFFUSIONTOOLS_API FStableDiffusionModelOptions
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Model")
		FString Model;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Model")
		FString Revision;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Model")
		FFilePath LocalFilePath;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Model")
		FString Precision = "fp16";

	FORCEINLINE bool operator==(const FStableDiffusionModelOptions& Other)
	{
		return Model.Equals(Other.Model) &&
			Revision.Equals(Other.Revision) &&
			Precision.Equals(Other.Precision);
	}

	FORCEINLINE bool operator!=(const FStableDiffusionModelOptions& Other)
	{
		return !Model.Equals(Other.Model) ||
			!Revision.Equals(Other.Revision) ||
			!Precision.Equals(Other.Precision);
	}
};


UCLASS()
class STABLEDIFFUSIONTOOLS_API UStableDiffusionModelAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()
public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Model")
		FStableDiffusionModelOptions Options;
};


USTRUCT(BlueprintType, meta = (UsesHierarchy = true))
struct STABLEDIFFUSIONTOOLS_API FStableDiffusionModelInitResult
{
	GENERATED_BODY()
public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Model")
		EModelStatus ModelStatus = EModelStatus::Unloaded;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Model")
		FString ErrorMsg;
};


USTRUCT(BlueprintType, meta=(UsesHierarchy=true))
struct STABLEDIFFUSIONTOOLS_API FStableDiffusionPipelineOptions
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pipeline")
	FString DiffusionPipeline;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pipeline")
	FString CustomPipeline;

	/*
	* Process and add any additional keyword arguments for the model pipe in Python at model init.
	* Any local variable in this script prefixed with the substring 'pipearg_' will be added to the pipe's keyword argument list
	*/
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (MultiLine = "true", Category = "Pipeline"))
	FString PythonModelArgumentsScript;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pipeline")
	FString Scheduler;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pipeline", meta = (Bitmask, BitmaskEnum = EPipelineCapabilities))
	int32 Capabilities = (int32)(EPipelineCapabilities::STRENGTH);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pipeline", meta = (Bitmask, BitmaskEnum = EPipelineCapabilities))
	TArray<FLayerData> RequiredLayers;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pipeline", meta = (Bitmask, BitmaskEnum = EPipelineCapabilities))
	TArray<FString> RequiredLayerKeys;

	FORCEINLINE bool operator==(const FStableDiffusionPipelineOptions& Other)
	{
		return DiffusionPipeline.Equals(Other.DiffusionPipeline) &&
			CustomPipeline.Equals(Other.CustomPipeline) &&
			Scheduler.Equals(Other.Scheduler) &&
			Capabilities == Other.Capabilities;
	}

	FORCEINLINE bool operator!=(const FStableDiffusionPipelineOptions& Other)
	{
		return !DiffusionPipeline.Equals(Other.DiffusionPipeline) ||
			!CustomPipeline.Equals(Other.CustomPipeline) ||
			!Scheduler.Equals(Other.Scheduler) ||
			!Capabilities == Other.Capabilities;
	}
};


UCLASS()
class STABLEDIFFUSIONTOOLS_API UStableDiffusionPipelineAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()
public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Pipeline")
		FStableDiffusionPipelineOptions Options;
};


USTRUCT(BlueprintType, meta = (UsesHierarchy = true))
struct STABLEDIFFUSIONTOOLS_API FStableDiffusionLORAOptions
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LORA")
		FString Model;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LORA")
		FString Revision;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LORA")
		FString ExternalURL;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LORA")
		FFilePath LocalFilePath;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LORA")
		TArray<FString> TriggerWords;

	bool IsValid() const {
		return !Model.IsEmpty() || !ExternalURL.IsEmpty();
	}
};

UCLASS()
class STABLEDIFFUSIONTOOLS_API UStableDiffusionLORAAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()
public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "LORA")
		FStableDiffusionLORAOptions Options;
};


USTRUCT(BlueprintType)
struct STABLEDIFFUSIONTOOLS_API FStableDiffusionGenerationOptions
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation options")
	float Strength = 0.75;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation options")
	float GuidanceScale = 7.5;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation options")
	int32 Iterations = 50;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation options")
	int32 Seed = -1;

	UPROPERTY(BlueprintReadWrite, Category = "Generation options")
	int32 InSizeX = -1;

	UPROPERTY(BlueprintReadWrite, Category = "Generation options")
	int32 InSizeY = -1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation options")
	int32 OutSizeX = 512;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation options")
	int32 OutSizeY = 512;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation options")
	TArray<FPrompt> PositivePrompts;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation options")
	TArray<FPrompt> NegativePrompts;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation options")
	float LoraWeight = 1.0f;

	void AddPrompt(const FPrompt Prompt){
		if (Prompt.Sentiment == EPromptSentiment::Negative) {
			NegativePrompts.Add(Prompt);
			return;
		}
		PositivePrompts.Add(Prompt);
	}
};


UCLASS()
class STABLEDIFFUSIONTOOLS_API UStableDiffusionGenerationAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()
public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Generation options")
	FStableDiffusionGenerationOptions Options;
};


USTRUCT(BlueprintType)
struct STABLEDIFFUSIONTOOLS_API FStableDiffusionInput
{
	GENERATED_BODY()
public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation")
	FStableDiffusionGenerationOptions Options;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation")
	USceneCaptureComponent2D* CaptureSource = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation")
	UTexture* OverrideTextureInput = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation")
	UTexture2D* TextureOutput = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation")
	TArray<FLayerData> InputLayers;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation")
	TArray<FLayerData> ProcessedLayers;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation")
	int32 PreviewIterationRate = 25;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation")
	bool DebugPythonImages = false;

	UPROPERTY(BlueprintReadWrite, Category = "Generation")
	FMinimalViewInfo View;
};
