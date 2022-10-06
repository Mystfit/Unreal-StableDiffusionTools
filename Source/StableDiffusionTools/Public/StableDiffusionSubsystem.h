// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "EditorSubsystem.h"
#include "FrameGrabber.h"
#include "Slate/SceneViewport.h"
#include "StableDiffusionBridge.h"
#include "StableDiffusionSubsystem.generated.h"

DECLARE_MULTICAST_DELEGATE_OneParam(FImageGenerationComplete, UTexture2D*);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FModelInitialized, bool, Success);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FDependenciesInstalled, bool, Success);

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
	void InitModel();

	UFUNCTION(BlueprintCallable)
	void StartCapturingViewport(FIntPoint FrameSize);

	UFUNCTION(BlueprintCallable)
	void GenerateImage(const FString& Prompt, FIntPoint Size, float InputStrength, int32 Iterations, int32 Seed);

	UPROPERTY(BlueprintReadOnly)
	bool ModelInitialised;

	UPROPERTY(EditAnywhere)
	TObjectPtr<UStableDiffusionBridge> GeneratorBridge;

	//PROPERTY(BlueprintAssignable, Category = "StableDiffusion")
	FImageGenerationComplete OnImageGenerationComplete;

	UPROPERTY(BlueprintAssignable, Category = "StableDiffusion")
	FModelInitialized OnModelInitialized;

	UPROPERTY(BlueprintAssignable, Category = "StableDiffusion")
	FDependenciesInstalled OnDependenciesInstalled;

private:
	void SetCaptureViewport(TSharedRef<FSceneViewport> Viewport, FIntPoint FrameSize);
	TSharedPtr<FFrameGrabber> ViewportCapture;

	FString CurrentBridgeID;
	
	FDelegateHandle ActiveEndframeHandler;
};
