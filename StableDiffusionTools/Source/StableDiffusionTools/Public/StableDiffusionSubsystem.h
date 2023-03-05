// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "EditorSubsystem.h"
#include "FrameGrabber.h"
#include "Slate/SceneViewport.h"
#include "Engine/SceneCapture2D.h"
#include "LevelEditorViewport.h"
#include "StableDiffusionBridge.h"
#include "DependencyManager.h"
#include "StableDiffusionImageResult.h"
#include "StableDiffusionSubsystem.generated.h"

DECLARE_MULTICAST_DELEGATE_ThreeParams(FFrameCaptureComplete, FColor*, FIntPoint, FIntPoint);

DECLARE_MULTICAST_DELEGATE_OneParam(FImageGenerationComplete, FStableDiffusionImageResult);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FImageGenerationCompleteEx, FStableDiffusionImageResult, Result);

DECLARE_MULTICAST_DELEGATE_OneParam(FModelInitialized, bool);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FModelInitializedEx, bool, Success);
DECLARE_MULTICAST_DELEGATE(FPythonLoaded);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FPythonLoadedEx);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FDependenciesInstalled, bool, Success);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FBridgeLoadedEx, UStableDiffusionBridge*, BridgeInstance);

struct FCapturedFramePayload : public IFramePayload {
	virtual bool OnFrameReady_RenderThread(FColor* ColorBuffer, FIntPoint BufferSize, FIntPoint TargetSize) const override;
	FFrameCaptureComplete OnFrameCapture;
};

struct FViewportSceneCapture {
	TObjectPtr<ASceneCapture2D> SceneCapture;
	TObjectPtr<FLevelEditorViewportClient> ViewportClient;
};

struct FStencilValues
{
	FStencilValues()
		: bRenderCustomDepth(false)
		, StencilMask(ERendererStencilMask::ERSM_Default)
		, CustomStencil(0)
	{
	}

	bool bRenderCustomDepth;
	ERendererStencilMask StencilMask;
	int32 CustomStencil;
};



USTRUCT(BlueprintType)
struct STABLEDIFFUSIONTOOLS_API FEditorCameraLivePreview
{
	GENERATED_BODY()
public:
	UPROPERTY(BlueprintReadWrite, Category = "StableDiffusion|Preview")
	FVector Location;

	UPROPERTY(BlueprintReadWrite, Category = "StableDiffusion|Preview")
	FRotator Rotation;

	UPROPERTY(BlueprintReadWrite, Category = "StableDiffusion|Preview")
	TEnumAsByte<ELevelViewportType> ViewportType;

	UPROPERTY(BlueprintReadWrite, Category = "StableDiffusion|Preview")
	int32 ViewportIndex;

	FORCEINLINE bool operator==(const FEditorCameraLivePreview& Other)
	{
		return Location.Equals(Other.Location, 0.001) &&
			Rotation.Equals(Other.Rotation, 0.001) &&
			ViewportType == Other.ViewportType &&
			ViewportIndex == Other.ViewportIndex;
	}
};
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnEditorCameraMovedEx, FEditorCameraLivePreview, CameraInfo);


///** Graph task for simple fire-and-forget asynchronous functions. */
//class FSDRenderTask
//	: public TAsyncGraphTask<void>
//{
//public:
//
//	/** Creates and initializes a new instance. */
//	FSDRenderTask(ENamedThreads::Type InDesiredThread, TUniqueFunction<void()>&& InFunction)
//		: TAsyncGraphTask()
//		, DesiredThread(InDesiredThread)
//		, Function(MoveTemp(InFunction))
//	{ }
//
//	/** Performs the actual task. */
//	void DoTask(ENamedThreads::Type CurrentThread, const FGraphEventRef& MyCompletionGraphEvent)
//	{
//		Function();
//	}
//
//	/** Returns the name of the thread that this task should run on. */
//	ENamedThreads::Type GetDesiredThread()
//	{
//		return DesiredThread;
//	}
//
//private:
//
//	/** The thread to execute the function on. */
//	ENamedThreads::Type DesiredThread;
//
//	/** The function to execute on the Task Graph. */
//	TUniqueFunction<void()> Function;
//};


/**
 * 
 */
UCLASS(BlueprintType)
class STABLEDIFFUSIONTOOLS_API UStableDiffusionSubsystem : public UEditorSubsystem
{
	GENERATED_BODY()
public:
	UStableDiffusionSubsystem(const FObjectInitializer& initializer);

	static FString NormalMaterialAsset;
	static FString StencilLayerMaterialAsset;
	


	// Python classes
	// --------------
	UFUNCTION(Category = "StableDiffusion|Bridge")
	void CreateBridge(TSubclassOf<UStableDiffusionBridge> BridgeClass);

	UFUNCTION(Category = "StableDiffusion|Bridge")
	bool IsBridgeLoaded();

	UPROPERTY(BlueprintAssignable, Category = "StableDiffusion|Bridge")
	FBridgeLoadedEx OnBridgeLoadedEx;

	UPROPERTY(EditAnywhere, Category="StableDiffusion|Generation")
	TObjectPtr<UStableDiffusionBridge> GeneratorBridge;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "StableDiffusion|Dependencies")
	TObjectPtr<UDependencyManager> DependencyManager;


	// Subsystem functions
	// -------------------

	UPROPERTY(BlueprintAssignable, Category = "StableDiffusion")
	FPythonLoadedEx OnPythonLoadedEx;

	FPythonLoaded OnPythonLoaded;

	UPROPERTY(BlueprintReadWrite, Category = "StableDiffusion")
	bool PythonLoaded = false;

	UFUNCTION(BlueprintCallable, Category = "StableDiffusion|Dependencies")
	bool DependenciesAreInstalled();

	UFUNCTION(BlueprintCallable, Category = "StableDiffusion|Dependencies")
	void RestartEditor();

	UFUNCTION(BlueprintCallable, Category = "StableDiffusion|Dependencies")
	void InstallDependency(FDependencyManifestEntry Dependency, bool ForceReinstall);

	UPROPERTY(BlueprintAssignable, Category = "StableDiffusion|Dependencies")
	FDependenciesInstalled OnDependenciesInstalled;

	UFUNCTION(BlueprintCallable, Category = "StableDiffusion|Model")
	bool HasToken();

	UFUNCTION(BlueprintCallable, Category = "StableDiffusion|Model")
	FString GetToken();

	UFUNCTION(BlueprintCallable, Category = "StableDiffusion|Model")
	bool LoginUsingToken(const FString& token);

	UFUNCTION(BlueprintCallable, Category = "StableDiffusion|Model")
	void InitModel(const FStableDiffusionModelOptions& Model, bool Async, bool AllowNSFW, EPaddingMode PaddingMode);

	UFUNCTION(BlueprintCallable, Category = "StableDiffusion|Model")
	void ReleaseModel();

	UFUNCTION(BlueprintCallable, Category = "StableDiffusion|Generation")
	void GenerateImage(FStableDiffusionInput Input, EInputImageSource ImageSourceType);

	UFUNCTION(BlueprintCallable, Category = "StableDiffusion|Generation")
	void StopGeneratingImage();

	UFUNCTION(BlueprintCallable, Category = "StableDiffusion|Outputs")
	void UpsampleImage(const FStableDiffusionImageResult& input);

	UPROPERTY(BlueprintAssignable, Category = "StableDiffusion|Outputs")
	FImageGenerationCompleteEx OnImageUpsampleCompleteEx;

	UFUNCTION(BlueprintCallable, Category = "StableDiffusion|Outputs")
	bool SaveTextureAsset(const FString& PackagePath, const FString& Name, UTexture2D* Texture, const FStableDiffusionGenerationOptions& ImageInputs, bool Upsampled = false);

	UFUNCTION(BlueprintCallable, Category = "StableDiffusion|Utilities")
	FString OpenImageFilePicker(const FString& StartDir);

	UFUNCTION(BlueprintCallable, Category = "StableDiffusion|Utilities")
	FString FilepathToLongPackagePath(const FString& Path);

	UPROPERTY(BlueprintReadOnly, Category = "StableDiffusion|Model")
	bool ModelInitialised;

	UPROPERTY(BlueprintAssignable, Category = "StableDiffusion|Model")
	FModelInitializedEx OnModelInitializedEx;

	UPROPERTY(BlueprintReadOnly, Category = "StableDiffusion|Model")
	FStableDiffusionModelOptions ModelOptions;

	UPROPERTY(BlueprintAssignable, Category = "StableDiffusion|Generation")
	FImageGenerationCompleteEx OnImageGenerationCompleteEx;

	UFUNCTION(BlueprintCallable, Category = "StableDiffusion|Preview")
	void SetLivePreviewEnabled(bool Enabled, float Delay = 0.5f, USceneCaptureComponent2D* CaptureSource = nullptr);

	UFUNCTION(BlueprintCallable, Category = "StableDiffusion|Preview")
	UTextureRenderTarget2D* SetLivePreviewForLayer(FIntPoint Size, ULayerProcessorBase* Layer, USceneCaptureComponent2D* CaptureSource = nullptr);

	UPROPERTY(BlueprintReadOnly, Category = "StableDiffusion|Preview")
	ULayerProcessorBase* PreviewedLayer;

	UFUNCTION(BlueprintCallable, Category = "StableDiffusion|Preview")
	void DisableLivePreviewForLayer();

	UPROPERTY(BlueprintAssignable, Category = "StableDiffusion|Preview")
	FOnEditorCameraMovedEx OnEditorCameraMovedEx;

	UPROPERTY(BlueprintAssignable, Category = "StableDiffusion|Generation")
	FImageProgressEx OnImageProgressUpdated;

	UPROPERTY(BlueprintReadOnly, Category = "StableDiffusion|Generation")
	bool bIsGenerating = false;

	UPROPERTY(BlueprintReadOnly, Category = "StableDiffusion|Generation")
	bool bIsUpsampling = false;

	UFUNCTION(BlueprintCallable, Category = "StableDiffusion|Outputs")
	UTexture2D* ColorBufferToTexture(const FString& FrameName, const TArray<FColor>& FrameColors, const FIntPoint& FrameSize, UTexture2D* OutTexture);

	TArray<FColor> CopyFrameData(FIntPoint TargetSize, FIntPoint BufferSize, FColor* ColorBuffer);

protected:
	UFUNCTION(Category = "StableDiffusion|Generation")
	void UpdateImageProgress(int32 Step, int32 Timestep, float Progress, FIntPoint Size, const TArray<FColor>& PixelData);

private:
	// Viewport capture
	TSharedPtr<FSceneViewport> GetCapturingViewport();
	void StartCapturingViewport();
	void SetCaptureViewport(TSharedRef<FSceneViewport> Viewport, FIntPoint FrameSize);
	TSharedPtr<FFrameGrabber> ViewportCapture;
	FDelegateHandle ActiveEndframeHandler;

	// Scene Capture Component capture
	FViewportSceneCapture CreateSceneCaptureCamera();
	void UpdateSceneCaptureCamera(FViewportSceneCapture& SceneCapture);
	FViewportSceneCapture CurrentSceneCapture;

	// Capture from the currently active viewport
	void CaptureFromViewportSource(FStableDiffusionInput Input);

	// Capture from a provided SceneCapture2D actor
	void CaptureFromSceneCaptureSource(FStableDiffusionInput Input);

	// Kick off an async render
	void StartImageGeneration(FStableDiffusionInput Input);

	FGraphEventRef CurrentRenderTask;

	UTexture2D* ColorBufferToTexture(const FString& FrameName, const uint8* FrameData, const FIntPoint& FrameSize, UTexture2D* OutTexture);

	// Python initialization
	FScriptDelegate OnPythonLoadedDlg;	

	// Live preview
	void LivePreviewUpdate();
	void OnLivePreviewCheckUpdate(USceneComponent* UpdatedComponent, EUpdateTransformFlags UpdateTransformFlags, ETeleportType Teleport);

	//FDelegateHandle OnEditorCameraUpdatedDlgHandle;
	FDelegateHandle OnCaptureCameraUpdatedDlgHandle;
	FEditorCameraLivePreview LastPreviewCameraInfo;
	FTimerHandle IdleCameraTimer;

	FViewportSceneCapture LayerPreviewCapture;
	FDelegateHandle OnLayerPreviewUpdateHandle;
};
