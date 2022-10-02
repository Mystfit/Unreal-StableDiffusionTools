// Fill out your copyright notice in the Description page of Project Settings.
#include "StableDiffusionSubsystem.h"
#include "IAssetViewport.h"
#include "Engine/GameEngine.h"
#include "LevelEditor.h"

void UStableDiffusionSubsystem::StartCapturingViewport(FIntPoint FrameSize)
{
	TSharedPtr<FSceneViewport> OutSceneViewport;

#if WITH_EDITOR
	if (GIsEditor)
	{
		for (const FWorldContext& Context : GEngine->GetWorldContexts())
		{
			if (Context.WorldType == EWorldType::Editor)
			{
				if (FModuleManager::Get().IsModuleLoaded("LevelEditor"))
				{
					FLevelEditorModule& LevelEditorModule = FModuleManager::GetModuleChecked<FLevelEditorModule>("LevelEditor");
					TSharedPtr<IAssetViewport> ActiveLevelViewport = LevelEditorModule.GetFirstActiveViewport();
					if (ActiveLevelViewport.IsValid())
					{
						OutSceneViewport = ActiveLevelViewport->GetSharedActiveViewport();
					}
				}
			}
			else if (Context.WorldType == EWorldType::PIE)
			{
				FSlatePlayInEditorInfo* SlatePlayInEditorSession = GEditor->SlatePlayInEditorMap.Find(Context.ContextHandle);
				if (SlatePlayInEditorSession)
				{
					if (SlatePlayInEditorSession->DestinationSlateViewport.IsValid())
					{
						TSharedPtr<IAssetViewport> DestinationLevelViewport = SlatePlayInEditorSession->DestinationSlateViewport.Pin();
						OutSceneViewport = DestinationLevelViewport->GetSharedActiveViewport();
					}
					else if (SlatePlayInEditorSession->SlatePlayInEditorWindowViewport.IsValid())
					{
						OutSceneViewport = SlatePlayInEditorSession->SlatePlayInEditorWindowViewport;
					}
				}
			}
		}
	}
	else
#endif
	{
		UGameEngine* GameEngine = Cast<UGameEngine>(GEngine);
		OutSceneViewport = GameEngine->SceneViewport;
	}

	SetCaptureViewport(OutSceneViewport.ToSharedRef(), FrameSize);
}


void UStableDiffusionSubsystem::SetCaptureViewport(TSharedRef<FSceneViewport> Viewport, FIntPoint FrameSize)
{
	if (ViewportCapture)
		ViewportCapture->StopCapturingFrames();

	ViewportCapture = MakeShared<FFrameGrabber>(Viewport, FrameSize);
	ViewportCapture->StartCapturingFrames();
}

UTexture2D* UStableDiffusionSubsystem::GenerateImage(const FString& Prompt, FIntPoint Size, float InputStrength, int32 Iterations, int32 Seed)
{
	if (!ViewportCapture)
		return nullptr;

	//const FString& prompt, const FIntPoint& FrameSize, const TArray<FColor>& GuideFrame, const TArray<FColor>& MaskFrame
	ViewportCapture->CaptureThisFrame(FFramePayloadPtr());
	
	TArray<FCapturedFrameData> CapturedFrames = ViewportCapture->GetCapturedFrames();
	if (CapturedFrames.Num()) {
		TArray<FColor> pixels = GeneratorBridge->GenerateImageFromStartImage(Prompt, Size.X, Size.Y, CapturedFrames[CapturedFrames.Num() - 1].ColorBuffer, TArray<FColor>(), InputStrength, Iterations, Seed);
		auto texture = GeneratorBridge->ColorBufferToTexture(Prompt, pixels, Size);
		OnImageGenerationComplete.Broadcast(texture);
		return texture;
	}

	return nullptr;
}
