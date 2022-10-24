// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "EditorSubsystem.h"
#include "FrameGrabber.h"
#include "Slate/SceneViewport.h"
#include "StableDiffusionBridge.h"
#include "StableDiffusionImageResult.h"
#include "StableDiffusionSubsystem.generated.h"

DECLARE_MULTICAST_DELEGATE_ThreeParams(FFrameCaptureComplete, FColor*, FIntPoint, FIntPoint);

DECLARE_MULTICAST_DELEGATE_OneParam(FImageGenerationComplete, FStableDiffusionImageResult);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FImageGenerationCompleteEx, FStableDiffusionImageResult, Result);

DECLARE_MULTICAST_DELEGATE_OneParam(FModelInitialized, bool);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FModelInitializedEx, bool, Success);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FDependenciesInstalled, bool, Success);


struct FCapturedFramePayload : public IFramePayload {
	virtual bool OnFrameReady_RenderThread(FColor* ColorBuffer, FIntPoint BufferSize, FIntPoint TargetSize) const override;
	FFrameCaptureComplete OnFrameCapture;
};


/**
 * 
 */
UCLASS(BlueprintType)
class STABLEDIFFUSIONTOOLS_API UStableDiffusionSubsystem : public UEditorSubsystem
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable, Category = "StableDiffusion|Dependencies")
	bool DependenciesAreInstalled();

	UFUNCTION(BlueprintCallable, Category = "StableDiffusion|Dependencies")
	void InstallDependencies();

	UFUNCTION(BlueprintCallable, Category = "StableDiffusion|Model")
	bool HasHuggingFaceToken();

	UFUNCTION(BlueprintCallable, Category = "StableDiffusion|Model")
	FString GetHuggingfaceToken();

	UFUNCTION(BlueprintCallable, Category = "StableDiffusion|Model")
	bool LoginHuggingFaceUsingToken(const FString& token);

	UFUNCTION(BlueprintCallable, Category = "StableDiffusion|Model")
	void InitModel(const FStableDiffusionModelOptions& Model, bool Async);

	UFUNCTION(BlueprintCallable, Category = "StableDiffusion|Model")
	void ReleaseModel();

	UFUNCTION(BlueprintCallable, Category = "StableDiffusion|Model")
	void StartCapturingViewport(FIntPoint Size);

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

	UPROPERTY(EditAnywhere, Category = "StableDiffusion|Generation")
	TObjectPtr<UStableDiffusionBridge> GeneratorBridge;

	UPROPERTY(BlueprintAssignable, Category = "StableDiffusion|Generation")
	FImageGenerationCompleteEx OnImageGenerationCompleteEx;

	UPROPERTY(BlueprintAssignable, Category = "StableDiffusion|Model")
	FModelInitializedEx OnModelInitializedEx;
	
	UPROPERTY(BlueprintAssignable, Category = "StableDiffusion|Dependencies")
	FDependenciesInstalled OnDependenciesInstalled;

	UPROPERTY(BlueprintAssignable, Category = "StableDiffusion|Generation")
	FImageProgressEx OnImageProgressUpdated;

	UFUNCTION(BlueprintCallable, Category = "StableDiffusion|Outputs")
	UTexture2D* ColorBufferToTexture(const FString& FrameName, const TArray<FColor>& FrameColors, const FIntPoint& FrameSize, UTexture2D* OutTexture);

protected:
	UFUNCTION(Category = "StableDiffusion|Generation")
	void UpdateImageProgress(int32 Step, int32 Timestep, FIntPoint Size, const TArray<FColor>& PixelData);

private:
	// Viewport capture
	void SetCaptureViewport(TSharedRef<FSceneViewport> Viewport, FIntPoint FrameSize);
	TSharedPtr<FFrameGrabber> ViewportCapture;
	FDelegateHandle ActiveEndframeHandler;

	TArray<FColor> CopyFrameData(FIntPoint TargetSize, FIntPoint BufferSize, FColor* ColorBuffer);
	UTexture2D* ColorBufferToTexture(const FString& FrameName, const uint8* FrameData, const FIntPoint& FrameSize, UTexture2D* OutTexture);
};
