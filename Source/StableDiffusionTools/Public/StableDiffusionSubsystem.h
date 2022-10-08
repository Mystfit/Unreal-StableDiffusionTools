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
//DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FImageProgressEx, int32, Step, int32, Timestep, UTexture2D*, Texture);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FModelInitialized, bool, Success);
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
	UFUNCTION(BlueprintCallable)
	bool DependenciesAreInstalled();

	UFUNCTION(BlueprintCallable)
	void InstallDependencies();

	UFUNCTION(BlueprintCallable)
	bool HasHuggingFaceToken();

	UFUNCTION(BlueprintCallable)
	bool LoginHuggingFaceUsingToken(const FString& token);

	UFUNCTION(BlueprintCallable)
	void InitModel();

	UFUNCTION(BlueprintCallable)
	void StartCapturingViewport(FIntPoint Size);

	UFUNCTION(BlueprintCallable)
	void GenerateImage(const FString& Prompt, FIntPoint Size, float InputStrength, int32 Iterations, int32 Seed);

	UFUNCTION(BlueprintCallable)
	bool SaveImageData(const FString& PackagePath, const FString& Name, UTexture2D* Texture);

	UPROPERTY(BlueprintReadOnly)
	bool ModelInitialised;

	UPROPERTY(EditAnywhere)
	TObjectPtr<UStableDiffusionBridge> GeneratorBridge;

	FImageGenerationComplete OnImageGenerationComplete;

	UPROPERTY(BlueprintAssignable, Category = "StableDiffusion")
	FImageGenerationCompleteEx OnImageGenerationCompleteEx;

	UPROPERTY(BlueprintAssignable, Category = "StableDiffusion")
	FModelInitialized OnModelInitialized;

	UPROPERTY(BlueprintAssignable, Category = "StableDiffusion")
	FDependenciesInstalled OnDependenciesInstalled;

	UPROPERTY(BlueprintAssignable, Category = "StableDiffusion")
	FImageProgressEx OnImageProgressUpdated;

	UFUNCTION()
	void UpdateImageProgress(int32 Step, int32 Timestep, FIntPoint Size, const TArray<FColor>& PixelData);

	UFUNCTION(BlueprintCallable)
	UTexture2D* ColorBufferToTexture(const FString& FrameName, const TArray<FColor>& FrameColors, const FIntPoint& FrameSize, UTexture2D* OutTexture);

	UTexture2D* ColorBufferToTexture(const FString& FrameName, const uint8* FrameData, const FIntPoint& FrameSize, UTexture2D* OutTexture);

private:
	void SetCaptureViewport(TSharedRef<FSceneViewport> Viewport, FIntPoint FrameSize);
	TSharedPtr<FFrameGrabber> ViewportCapture;

	FString CurrentBridgeID;
	
	FDelegateHandle ActiveEndframeHandler;
};
