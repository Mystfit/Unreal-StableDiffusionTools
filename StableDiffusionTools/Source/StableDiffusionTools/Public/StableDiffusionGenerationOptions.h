#pragma once

#include "CoreMinimal.h"
#include "PromptAsset.h"
#include "ActorLayerUtilities.h"
//#include "LayerProcessorBase.h"
#include "Components/SceneCaptureComponent2D.h"

#include "StableDiffusionGenerationOptions.generated.h"

UENUM(BlueprintType, meta = (Bitflags, UseEnumValuesAsMaskValuesInEditor = "true"))
enum class EPipelineCapabilities : uint8 {
	None = 0 UMETA(Hidden),
	ITERATIONS = 0x01,
	PROGRESS_UPDATES = 0x02,
	STRENGTH = 0x04,
	SEED = 0x08,
	POOLED_EMBEDDINGS = 0x10,
	NO_PROMPT_WEIGHTS = 0x20,
	GUIDANCE_SCALE = 0x40,
	NO_PROMPTS = 0x80
};
ENUM_CLASS_FLAGS(EPipelineCapabilities);

UENUM(BlueprintType, meta = (Bitflags, UseEnumValuesAsMaskValuesInEditor = "true"))
enum class EPipelineLORACapabilities : uint8 {
	None = 0 UMETA(Hidden),
	SCALE = 0x01,
};
ENUM_CLASS_FLAGS(EPipelineLORACapabilities);

UENUM(BlueprintType)
enum class EInputImageSource : uint8 {
	Viewport UMETA(DisplayName = "Viewport"),
	SceneCapture2D UMETA(DisplayName = "Scene Capture Actor"),
	Texture UMETA(DisplayName = "Texture")
};
ENUM_CLASS_FLAGS(EInputImageSource);

UENUM(BlueprintType)
enum class EImageType : uint8 {
	Image UMETA(DisplayName = "Image"),
	Latent UMETA(DisplayName = "Latent")
};
ENUM_CLASS_FLAGS(EImageType);

UENUM(BlueprintType, meta = (Bitflags, UseEnumValuesAsMaskValuesInEditor = "true"))
enum class ESeamlessMode : uint8 {
	//'zeros', 'reflect', 'replicate' or 'circular'.
	none = 0 UMETA(Hidden),
	seamless_x = 0x01 UMETA(DisplayName = "Seamless horizontal"),
	seamless_y = 0x02 UMETA(DisplayName = "Seamless vertical")
};
ENUM_CLASS_FLAGS(ESeamlessMode);

UENUM(BlueprintType)
enum class EModelStatus : uint8 {
	Unloaded UMETA(DisplayName = "Unloaded"),
	Loading UMETA(DisplayName = "Loading"),
	Loaded UMETA(DisplayName = "Loaded"),
	Downloading UMETA(DisplayName = "Downloading"),
	Cancelling UMETA(DisplayName = "Cancelling"),
	Error UMETA(DisplayName = "Error")
};
ENUM_CLASS_FLAGS(EModelStatus);

UENUM(BlueprintType)
enum class EModelType : uint8 {
	Checkpoint UMETA(DisplayName = "Checkpoint"),
	Diffusers UMETA(DisplayName = "Diffusers"),
	Placeholder UMETA(DisplayName = "Placeholder")
};
ENUM_CLASS_FLAGS(EModelType);

UENUM()
enum ELayerBitDepth
{
	EightBit UMETA(DisplayName = "8 bit channel bit depth"),
	SixteenBit UMETA(DisplayName = "16 bit channel bit depth"),
	LayerBitDepth_MAX
};

UENUM(BlueprintType)
enum class ELayerImageType : uint8
{
	unknown UMETA(DisplayName = "Unknown", Hidden),
	image UMETA(DisplayName = "Img2Img"),
	utility UMETA(DisplayName = "Utility image"),
	control_image UMETA(DisplayName = "ControlNet"),
	custom UMETA(DisplayName = "Custom")
};
ENUM_CLASS_FLAGS(ELayerImageType);

UENUM(BlueprintType)
enum class EPipelineOutputTextureFormat : uint8
{
	BGRA UMETA(DisplayName = "BGRA"),
	FloatRGBA UMETA(DisplayName = "FloatRGBA")
};
ENUM_CLASS_FLAGS(EPipelineOutputTextureFormat);


// Forwards
class ULayerProcessorBase;
class ULayerProcessorOptions;

USTRUCT(BlueprintType)
struct STABLEDIFFUSIONTOOLS_API FLayerProcessorContext
{
	GENERATED_USTRUCT_BODY()
public:
	UPROPERTY(BlueprintReadWrite, Transient)
		TArray<FColor> LayerPixels;

	UPROPERTY(BlueprintReadWrite, Transient)
		TArray<uint8> LatentData;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (Category = "Layers"))
		UTexture2D* LayerTexture = nullptr;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Layers")
		UStableDiffusionControlNetModelAsset* Model = nullptr;

	UPROPERTY(BlueprintReadWrite, Instanced, EditAnywhere, meta = (Category = "Layers"))
		ULayerProcessorBase* Processor = nullptr;

	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Instanced, meta = (Category = "Layers", EditCondition = "Processor != nullptr", EditConditionHides))
		TObjectPtr<ULayerProcessorOptions> ProcessorOptions = nullptr;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (UsesHierarchy = true, Category = "Layers"))
		TEnumAsByte<ELayerImageType> LayerType = ELayerImageType::unknown;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (Category = "Layers"))
		TEnumAsByte<EImageType> OutputType = EImageType::Image;

	/*
	* Identifying role key for this layer that will be used to assign it to the correct pipe keyword argument
	*/
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (Category = "Layers", EditCondition = "LayerType == ELayerImageType::custom", EditConditionHides))
		FString Role = "image";
};



USTRUCT(BlueprintType, meta=(UsesHierarchy=true))
struct STABLEDIFFUSIONTOOLS_API FStableDiffusionModelOptions
{
	GENERATED_BODY()
public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Model")
		FString Model;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Model")
		UStableDiffusionModelAsset* BaseModel;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Model")
		FString Revision;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Model")
		FString Precision = "fp16";

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Model")
		FIntPoint BaseResolution = FIntPoint(512, 512);

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Model")
		FString ExternalURL;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Model")
		EModelType ModelType;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (Category = "Model", EditCondition = "ModelType == EModelType::Checkpoint", EditConditionHides))
		FFilePath LocalFilePath;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (Category = "Model", EditCondition = "ModelType == EModelType::Diffusers", EditConditionHides))
		FDirectoryPath LocalFolderPath;

	bool IsValid() const {
		return !Model.IsEmpty() || !ExternalURL.IsEmpty();
	}

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


// Forward declarations
class UStableDiffusionModelAsset;


UCLASS(EditInlineNew)
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

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Model")
		FString ModelName;
};


USTRUCT(BlueprintType, meta=(UsesHierarchy=true))
struct STABLEDIFFUSIONTOOLS_API FStableDiffusionPipelineOptions
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pipeline")
	FString PipelineModule = "diffusers";

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pipeline")
	FString DiffusionPipeline;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pipeline")
	FString CustomPipeline;

	/*
	* Process and add any additional keyword arguments for the pipeline in Python at model init.
	* Any local variable in this script prefixed with the substring 'pipearg_' will be added to the pipe's keyword argument list
	*/
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (MultiLine = "true", Category = "Pipeline"))
	FString PythonModelArgumentsScript;

	/*
	* Process any python commands that need to be run after the pipeline is initialised.
	*/
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (MultiLine = "true", Category = "Pipeline"))
		FString PythonPipelinePostInitScript;

	/*
	* Process the image further after it was generated.
	* 'input' contains the current input options passed to the generation function
	* 'generation_args' are the arguments that have been created already
	* 'input_image' variable is the input image. Modify it and set the 'result_image' local variable in the script after processing.
	*/
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (MultiLine = "true", Category = "Pipeline"))
		FString PythonPreRenderScript;

	/*
	* Process the image further after it was generated.
	* 'input_image' variable is the input image. Modify it and set the 'result_image' local variable in the script after processing.
	*/
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (MultiLine = "true", Category = "Pipeline"))
	FString PythonPostRenderScript;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(Category = "Scheduler", Tooltip="Choose an available from the currently loaded model"))
	FString Scheduler;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pipeline", meta = (Bitmask, BitmaskEnum = "/Script/StableDiffusionTools.EPipelineCapabilities"))
	int32 Capabilities = (int32)(EPipelineCapabilities::STRENGTH);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pipeline")
	TArray<FLayerProcessorContext> RequiredLayers;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pipeline")
	TArray<FString> RequiredLayerKeys;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pipeline")
	EPipelineOutputTextureFormat OutputTextureFormat = EPipelineOutputTextureFormat::BGRA;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pipeline")
	bool EnableTiling = true;

	/*
	* Should this pipeline consume the layers passed in or pass them along to the next pipeline stage?
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pipeline")
	bool bConsumesLayers = true;

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


UCLASS(Blueprintable, EditInlineNew)
class STABLEDIFFUSIONTOOLS_API UStableDiffusionPipelineAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()
public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Pipeline")
	FStableDiffusionPipelineOptions Options;
};


USTRUCT(BlueprintType, meta = (UsesHierarchy = true))
struct STABLEDIFFUSIONTOOLS_API FStableDiffusionTriggerOptions
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LORA")
		TArray<FString> TriggerWords;
};


UCLASS(EditInlineNew)
class STABLEDIFFUSIONTOOLS_API UStableDiffusionTextualInversionAsset : public UStableDiffusionModelAsset
{
	GENERATED_BODY()
public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "LORA")
	FStableDiffusionTriggerOptions TriggerOptions;
};


UCLASS(EditInlineNew)
class STABLEDIFFUSIONTOOLS_API UStableDiffusionLORAAsset : public UStableDiffusionModelAsset
{
	GENERATED_BODY()
public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "LORA")
	FStableDiffusionTriggerOptions TriggerOptions;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pipeline", meta = (Bitmask, BitmaskEnum = "/Script/StableDiffusionTools.EPipelineLORACapabilities", Display = "LORA Capabilities"))
	int32 LORACapabilities = (int32)(EPipelineLORACapabilities::SCALE);
};


UCLASS(Blueprintable, EditInlineNew, CollapseCategories) //CollapseCategories , hidecategories = UObject
class STABLEDIFFUSIONTOOLS_API UStableDiffusionControlNetModelAsset : public UStableDiffusionModelAsset
{
	GENERATED_BODY()
public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Layer processors")
	TSubclassOf<ULayerProcessorBase> DefaultLayerProcessor = nullptr;
};


UCLASS()
class STABLEDIFFUSIONTOOLS_API UStableDiffusionStyleModelAsset : public UStableDiffusionModelAsset
{
	GENERATED_BODY()
};



USTRUCT(BlueprintType)
struct STABLEDIFFUSIONTOOLS_API FStableDiffusionGenerationOptions
{
	GENERATED_BODY()
public:
	//UPROPERTY(BlueprintReadWrite, meta = (Category = "Generation options"))
	//	bool IsMasterOptions = true;

	//UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (Category = "Generation options", EditCondition = "!IsMasterOptions", EditConditionHides))
	//	bool AllowOverrides = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (Category = "Generation options", PinHiddenByDefault, InlineEditConditionToggle))
		bool OverrideStrength = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (Category = "Generation options", EditCondition = "OverrideStrength"))
		float Strength = 0.75;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (Category = "Generation options", PinHiddenByDefault, InlineEditConditionToggle))
		bool OverrideGuidanceScale = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (Category = "Generation options", EditCondition = "OverrideGuidanceScale"))
		float GuidanceScale = 7.5;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (Category = "Generation options", PinHiddenByDefault, InlineEditConditionToggle))
		bool OverrideLoraWeight = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (Category = "Generation options", EditCondition = "OverrideLoraWeight"))
		float LoraWeight = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (Category = "Generation options", PinHiddenByDefault, InlineEditConditionToggle))
		bool OverrideIterations = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (Category = "Generation options", EditCondition = "OverrideIterations"))
		int32 Iterations = 50;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (Category = "Generation options", PinHiddenByDefault, InlineEditConditionToggle))
		bool OverrideSeed = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (Category = "Generation options", EditCondition = "OverrideSeed"))
		bool RandomSeed = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (Category = "Generation options", EditCondition = "!RandomSeed", EditConditionHides))
		int32 Seed = 0;

	UPROPERTY(BlueprintReadWrite, meta = (Category = "Generation options"))
		int32 InSizeX = -1;
	
	UPROPERTY(BlueprintReadWrite, meta = (Category = "Generation options"))
		int32 InSizeY = -1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (Category = "Generation options", PinHiddenByDefault, InlineEditConditionToggle, EditConditionHides))
		bool OverrideOutSizeX = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (DisplayName="Image width", Category = "Generation options", EditCondition = "OverrideOutSizeX"))
		int32 OutSizeX = 512;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (Category = "Generation options", PinHiddenByDefault, InlineEditConditionToggle, EditConditionHides))
		bool OverrideOutSizeY = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (DisplayName= "Image height", Category = "Generation options", EditCondition = "OverrideOutSizeY"))
		int32 OutSizeY = 512;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (Category = "Generation options", PinHiddenByDefault, InlineEditConditionToggle, EditConditionHides))
		bool OverridePositivePrompts = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (Category = "Generation options", EditCondition = "OverridePositivePrompts"))
		TArray<FPrompt> PositivePrompts;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (Category = "Generation options", PinHiddenByDefault, InlineEditConditionToggle, EditConditionHides))
		bool OverrideNegativePrompts = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (Category = "Generation options", EditCondition = "OverrideNegativePrompts"))
		TArray<FPrompt> NegativePrompts;

	//UPROPERTY(BlueprintReadWrite, Category = "Generation options")
	//	TEnumAsByte<EPipelineOutputType> OutputType;


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

	//UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation")
	//TArray<FLayerProcessorContext> InputLayers;

	//UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation")
	//TArray<FLayerProcessorContext> ProcessedLayers;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation")
	int32 PreviewIterationRate = 25;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation")
	bool DebugPythonImages = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation")
	bool SaveLayers = false;

	UPROPERTY(BlueprintReadWrite, Category = "Generation")
	FMinimalViewInfo View;

	UPROPERTY(BlueprintReadWrite, Category = "Generation")
		TEnumAsByte<EImageType> OutputType;
};


UCLASS(Blueprintable, EditInlineNew, AutoCollapseCategories = ("Stages", "Stage Layers", "Stage Overrides"))
class STABLEDIFFUSIONTOOLS_API UImagePipelineStageAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()
public:
	UImagePipelineStageAsset();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stage Models")
		UStableDiffusionStyleModelAsset* Model = nullptr;

	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Stage Models")
		void LoadModel();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Instanced, Category = "Stage Pipeline")
		UStableDiffusionPipelineAsset* Pipeline = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (Category = "Stage Overrides", GetOptions = "GetCompatibleSchedulers", Tooltip = "Choose a compatible scheduler from the model for this pipeline stage"))
		FString Scheduler;

	UFUNCTION(BlueprintCallable, Category = "Stage Scheduler")
		TArray<FString> GetCompatibleSchedulers();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stage Models")
		UStableDiffusionLORAAsset* LORAAsset = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stage Models")
		UStableDiffusionTextualInversionAsset* TextualInversionAsset = nullptr;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ShowOnlyInnerProperties, Category = "Stage Layers"))
		TArray<FLayerProcessorContext> Layers;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ShowOnlyInnerProperties, Category = "Stage Output"))
		TEnumAsByte<EImageType> OutputType;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(Category = "Stage Overrides"))
		FStableDiffusionGenerationOptions OverrideInputOptions;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (Category = "Stage Output", Tooltip = "Don't include the image generated from this pipeline stage when calculating the last main image returned from the pipeline."))
		bool bSkipMainOutput;

	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;

private:
	TArray<FString> CompatibleSchedulers;
	FString LastQueriedModel;
};



UCLASS(Blueprintable, EditInlineNew, AutoCollapseCategories=("Stages"))
class STABLEDIFFUSIONTOOLS_API UImagePipelineAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Instanced, Category = "Stages")
	TArray<UImagePipelineStageAsset*> Stages;
private:
};

