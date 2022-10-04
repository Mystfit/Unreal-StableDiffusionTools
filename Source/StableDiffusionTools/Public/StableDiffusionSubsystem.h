// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "EditorSubsystem.h"
#include "FrameGrabber.h"
#include "Slate/SceneViewport.h"
#include "StableDiffusionBridge.h"
#include "StableDiffusionSubsystem.generated.h"

DECLARE_MULTICAST_DELEGATE_OneParam(FImageGenerationComplete, UTexture2D*);
/**
 * 
 */
UCLASS(BlueprintType)
class STABLEDIFFUSIONTOOLS_API UStableDiffusionSubsystem : public UEditorSubsystem
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable)
	void StartCapturingViewport(FIntPoint FrameSize);

	UFUNCTION(BlueprintCallable)
	void GenerateImage(const FString& Prompt, FIntPoint Size, float InputStrength, int32 Iterations, int32 Seed);

	UPROPERTY(EditAnywhere)
	TObjectPtr<UStableDiffusionBridge> GeneratorBridge;

	//UPROPERTY(BlueprintAssignable, Category = "StableDiffusion")
	FImageGenerationComplete OnImageGenerationComplete;

private:
	void SetCaptureViewport(TSharedRef<FSceneViewport> Viewport, FIntPoint FrameSize);
	TSharedPtr<FFrameGrabber> ViewportCapture;
	
	FDelegateHandle ActiveEndframeHandler;
};
