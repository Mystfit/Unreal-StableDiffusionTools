#pragma once

#include "CoreMinimal.h"
#include "PromptAsset.h"
#include "ActorLayerUtilities.h"
#include "LayerProcessorBase.h"

#include "StableDiffusionGenerationOptions.generated.h"

UENUM(BlueprintType, meta = (Bitflags, UseEnumValuesAsMaskValuesInEditor = "true"))
enum class EModelCapabilities : uint8 {
	None = 0 UMETA(Hidden),
	INPAINT = 0x01,
	DEPTH = 0x02,
	STRENGTH = 0x04,
	CONTROL = 0x08
};
ENUM_CLASS_FLAGS(EModelCapabilities);

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
	Downloading UMETA(DisplayName = "Downloading")
};
ENUM_CLASS_FLAGS(EModelStatus);


USTRUCT(BlueprintType, meta=(UsesHierarchy=true))
struct STABLEDIFFUSIONTOOLS_API FStableDiffusionModelOptions
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stable Diffusion|Model")
		FString Model = "CompVis/stable-diffusion-v1-4";

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stable Diffusion|Model")
		FString Revision;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stable Diffusion|Model")
		FString Precision = "fp16";

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stable Diffusion|Model")
		FString DiffusionPipeline = "StableDiffusionImg2ImgPipeline";
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stable Diffusion|Model")
		FString CustomPipeline = "lpw_stable_diffusion";

	/*
	* Process and add any additional keyword arguments for the model pipe in Python at model init.
	* Any local variable in this script prefixed with the substring 'pipearg_' will be added to the pipe's keyword argument list
	*/
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (MultiLine = "true", Category = "Stable Diffusion|Model"))
		FString PythonModelArgumentsScript;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stable Diffusion|Model")
		FString Scheduler;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stable Diffusion|Model", meta = (Bitmask, BitmaskEnum = EModelCapabilities))
		int32 Capabilities = (int32)(EModelCapabilities::STRENGTH);

	//UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (UsesHierarchy=true, Category="Stable Diffusion|Model"))
	//TArray<FLayerData> Layers;

	FORCEINLINE bool operator==(const FStableDiffusionModelOptions& Other)
	{
		return Model.Equals(Other.Model) &&
			Revision.Equals(Other.Revision) &&
			Precision.Equals(Other.Precision) &&
			CustomPipeline.Equals(Other.CustomPipeline) &&
			Capabilities == Other.Capabilities;
	}

	FORCEINLINE bool operator!=(const FStableDiffusionModelOptions& Other)
	{
		return !Model.Equals(Other.Model) ||
			!Revision.Equals(Other.Revision) ||
			!Precision.Equals(Other.Precision) ||
			!CustomPipeline.Equals(Other.CustomPipeline) ||
			!(Capabilities == Other.Capabilities);
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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stable Diffusion|Generation")
	FStableDiffusionGenerationOptions Options;

	/*UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stable Diffusion|Generation")
	TArray<FActorLayer> InpaintLayers;*/

	/*
	//
	//Padding mode to use for image 2D convulution. Valid options are 'zeros', 'reflect', 'replicate' or 'circular'.
	//See https://pytorch.org/docs/stable/generated/torch.nn.Conv2d.html
	//
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stable Diffusion|Model")
	EPaddingMode PaddingMode;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stable Diffusion|Model")
	bool AllowNSFW = false;
	*/

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stable Diffusion|Generation")
	USceneCaptureComponent2D* CaptureSource;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stable Diffusion|Generation")
	UTexture* OverrideTextureInput;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stable Diffusion|Generation")
	UTexture2D* TextureOutput;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stable Diffusion|Generation")
	TArray<FLayerData> InputLayers;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stable Diffusion|Generation")
	TArray<FLayerData> ProcessedLayers;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stable Diffusion|Generation")
	int32 PreviewIterationRate = 25;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stable Diffusion|Generation")
	bool DebugPythonImages = false;

	UPROPERTY(BlueprintReadWrite, Category = "StableDiffusion|Generation")
	FMinimalViewInfo View;
};
