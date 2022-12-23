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

struct STABLEDIFFUSIONTOOLS_API FScopedActorLayerStencil {
public:
	FScopedActorLayerStencil() = delete;
	FScopedActorLayerStencil(const FScopedActorLayerStencil& ref);
	FScopedActorLayerStencil(const FActorLayer& Layer, bool RestoreOnDelete=true);
	~FScopedActorLayerStencil();

	void Restore();

private:
	bool RestoreOnDelete;

	// Stencil values
	TMap<UPrimitiveComponent*, FStencilValues> ActorLayerSavedStencilValues;

	// Cache the custom stencil value.
	TOptional<int32> PreviousCustomDepthValue;
};

/**
 * 
 */
UCLASS(BlueprintType)
class STABLEDIFFUSIONTOOLS_API UStableDiffusionSubsystem : public UEditorSubsystem
{
	GENERATED_BODY()
public:
	UStableDiffusionSubsystem(const FObjectInitializer& initializer);

	static FString StencilLayerMaterialAsset;
	static FString DepthMaterialAsset;
	


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
	void InitModel(const FStableDiffusionModelOptions& Model, bool Async);

	UFUNCTION(BlueprintCallable, Category = "StableDiffusion|Model")
	void ReleaseModel();

	UFUNCTION(BlueprintCallable, Category = "StableDiffusion|Generation")
	void GenerateImage(FStableDiffusionInput Input, bool FromViewport = true);

	UFUNCTION(BlueprintCallable, Category = "StableDiffusion|Outputs")
	void UpsampleImage(const FStableDiffusionImageResult& input);

	UPROPERTY(BlueprintAssignable, Category = "StableDiffusion|Outputs")
	FImageGenerationCompleteEx OnImageUpsampleCompleteEx;

	UFUNCTION(BlueprintCallable, Category = "StableDiffusion|Outputs")
	bool SaveTextureAsset(const FString& PackagePath, const FString& Name, UTexture2D* Texture);

	UFUNCTION(BlueprintCallable, Category = "StableDiffusion|Utilities")
	FString OpenImageFilePicker(const FString& StartDir);

	UFUNCTION(BlueprintCallable, Category = "StableDiffusion|Utilities")
	FString FilepathToLongPackagePath(const FString& Path);

	UPROPERTY(BlueprintReadOnly, Category = "StableDiffusion|Model")
	bool ModelInitialised;

	UPROPERTY(BlueprintReadOnly, Category = "StableDiffusion|Model")
	FStableDiffusionModelOptions ModelOptions;

	UPROPERTY(BlueprintAssignable, Category = "StableDiffusion|Generation")
	FImageGenerationCompleteEx OnImageGenerationCompleteEx;

	UPROPERTY(BlueprintAssignable, Category = "StableDiffusion|Model")
	FModelInitializedEx OnModelInitializedEx;

	UPROPERTY(BlueprintAssignable, Category = "StableDiffusion|Generation")
	FImageProgressEx OnImageProgressUpdated;

	UFUNCTION(BlueprintCallable, Category = "StableDiffusion|Outputs")
	UTexture2D* ColorBufferToTexture(const FString& FrameName, const TArray<FColor>& FrameColors, const FIntPoint& FrameSize, UTexture2D* OutTexture);

	TArray<FColor> CopyFrameData(FIntPoint TargetSize, FIntPoint BufferSize, FColor* ColorBuffer);

protected:
	UFUNCTION(Category = "StableDiffusion|Generation")
	void UpdateImageProgress(int32 Step, int32 Timestep, FIntPoint Size, const TArray<FColor>& PixelData);

private:
	// Viewport capture
	TSharedPtr<FSceneViewport> GetCapturingViewport();
	void SetCaptureViewport(TSharedRef<FSceneViewport> Viewport, FIntPoint FrameSize);
	TSharedPtr<FFrameGrabber> ViewportCapture;
	FDelegateHandle ActiveEndframeHandler;

	FViewportSceneCapture CreateSceneCaptureCamera();
	void UpdateSceneCaptureCamera(FViewportSceneCapture& SceneCapture);
	FViewportSceneCapture CurrentSceneCapture;

	UTexture2D* ColorBufferToTexture(const FString& FrameName, const uint8* FrameData, const FIntPoint& FrameSize, UTexture2D* OutTexture);

	// Python initialization
	FScriptDelegate OnPythonLoadedDlg;
};
