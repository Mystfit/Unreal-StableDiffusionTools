// Fill out your copyright notice in the Description page of Project Settings.
#include "StableDiffusionSubsystem.h"
#include "IAssetViewport.h"
#include "Engine/GameEngine.h"
#include "Async/Async.h"
#include "LevelEditor.h"
#include "IPythonScriptPlugin.h"
#include "PythonScriptTypes.h"
#include "LevelEditorSubsystem.h"
#include "StableDiffusionImageResult.h"
#include "AssetRegistryModule.h"
#include "ImageUtils.h"

bool FCapturedFramePayload::OnFrameReady_RenderThread(FColor* ColorBuffer, FIntPoint BufferSize, FIntPoint TargetSize) const
{
	OnFrameCapture.Broadcast(ColorBuffer, BufferSize, TargetSize);
	return true;
}

bool UStableDiffusionSubsystem::DependenciesAreInstalled()
{
	FPythonCommandEx PythonCommand;
	PythonCommand.Command = FString("SD_dependencies_installed");
	PythonCommand.ExecutionMode = EPythonCommandExecutionMode::EvaluateStatement;
	IPythonScriptPlugin::Get()->ExecPythonCommandEx(PythonCommand);
	return PythonCommand.CommandResult == "True";
}

void UStableDiffusionSubsystem::InstallDependencies() 
{
	AsyncTask(ENamedThreads::AnyBackgroundHiPriTask, [this]() {
		FPythonCommandEx PythonCommand;
		PythonCommand.Command = FString("install_dependencies.py");
		PythonCommand.ExecutionMode = EPythonCommandExecutionMode::ExecuteFile;
		
		bool install_result = IPythonScriptPlugin::Get()->ExecPythonCommandEx(PythonCommand);
		if (!install_result) {
			// Dependency installation failed - log result
			OnDependenciesInstalled.Broadcast(install_result);
			return;
		}

		AsyncTask(ENamedThreads::GameThread, [this]() {
			FPythonCommandEx PythonCommand;
			PythonCommand.Command = FString("load_diffusers_bridge.py");
			PythonCommand.ExecutionMode = EPythonCommandExecutionMode::ExecuteFile;
			bool reload_result = IPythonScriptPlugin::Get()->ExecPythonCommandEx(PythonCommand);
			OnDependenciesInstalled.Broadcast(reload_result);
		});
	});
}

bool UStableDiffusionSubsystem::HasHuggingFaceToken()
{
	FPythonCommandEx PythonCommand;
	PythonCommand.Command = FString("huggingface_hub.utils.HfFolder.get_token()");
	PythonCommand.ExecutionMode = EPythonCommandExecutionMode::EvaluateStatement;
	IPythonScriptPlugin::Get()->ExecPythonCommandEx(PythonCommand);
	return PythonCommand.CommandResult != "None";
}


bool UStableDiffusionSubsystem::LoginHuggingFaceUsingToken(const FString& token)
{
	FPythonCommandEx PythonCommand;
	PythonCommand.Command = FString::Format(TEXT("huggingface_hub.utils.HfFolder.save_token('{0}')"), { token });
	PythonCommand.ExecutionMode = EPythonCommandExecutionMode::ExecuteStatement;
	return IPythonScriptPlugin::Get()->ExecPythonCommandEx(PythonCommand);
}

void UStableDiffusionSubsystem::InitModel()
{
	if (GeneratorBridge) {
		AsyncTask(ENamedThreads::AnyBackgroundHiPriTask, [this](){
			this->ModelInitialised = this->GeneratorBridge->InitModel();
			AsyncTask(ENamedThreads::GameThread, [this]() {
				this->OnModelInitialized.Broadcast(this->ModelInitialised);
			});
		});
	}
}

void UStableDiffusionSubsystem::StartCapturingViewport(FIntPoint Size)
{
	// Find active viewport
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

	SetCaptureViewport(OutSceneViewport.ToSharedRef(), OutSceneViewport->GetSize());
}


void UStableDiffusionSubsystem::SetCaptureViewport(TSharedRef<FSceneViewport> Viewport, FIntPoint FrameSize)
{
	ViewportCapture = MakeShared<FFrameGrabber>(Viewport, FrameSize);
	ViewportCapture->StartCapturingFrames();
}

void UStableDiffusionSubsystem::GenerateImage(const FString& Prompt, FIntPoint Size, float InputStrength, int32 Iterations, int32 Seed)
{
	// Remember prior screen message state and disable it so our viewport is clean
	bool bPrevGScreenMessagesEnabled = GAreScreenMessagesEnabled;
	bool bPrevViewportGameViewEnabled = false;
	GAreScreenMessagesEnabled = false;
	ULevelEditorSubsystem* LevelEditorSubsystem = nullptr;

#if WITH_EDITOR
	//Only set Game view when streaming in editor mode (so not on PIE, SIE or standalone) 
	if (GEditor && !GEditor->IsPlaySessionInProgress())
	{
		LevelEditorSubsystem = GEditor->GetEditorSubsystem<ULevelEditorSubsystem>();
		if (LevelEditorSubsystem)
		{
			bPrevViewportGameViewEnabled = LevelEditorSubsystem->EditorGetGameView();
			LevelEditorSubsystem->EditorSetGameView(true);
		}
	}

#endif

	StartCapturingViewport(Size);

	if (!ViewportCapture || ActiveEndframeHandler.IsValid())
		return;

	// Forward image updated event from bridge to subsystem
	this->GeneratorBridge->OnImageProgressEx.AddUniqueDynamic(this, &UStableDiffusionSubsystem::UpdateImageProgress);

	// Create a frame payload we will wait on to be filled with a frame
	auto framePtr = MakeShared<FCapturedFramePayload>();
	framePtr->OnFrameCapture.AddLambda([this, Prompt, Size, InputStrength, Iterations, Seed, bPrevGScreenMessagesEnabled, bPrevViewportGameViewEnabled, LevelEditorSubsystem](FColor* Pixels, FIntPoint BufferSize, FIntPoint TargetSize)
	{
		// Restore screen messages and UI
		GAreScreenMessagesEnabled = bPrevGScreenMessagesEnabled;
		if(LevelEditorSubsystem)
			LevelEditorSubsystem->EditorSetGameView(bPrevViewportGameViewEnabled);

		// Copy frame data
		TArray<FColor> CopiedFrame;
		CopiedFrame.InsertUninitialized(0, TargetSize.X * TargetSize.Y);
		FColor* Dest = &CopiedFrame[0];
		//FColor* PixelPtr = Pixels;
		const int32 MaxWidth = FMath::Min(TargetSize.X, BufferSize.X);
		for (int32 Row = 0; Row < FMath::Min(TargetSize.Y, BufferSize.Y); ++Row)
		{
			FMemory::Memcpy(Dest, Pixels, sizeof(FColor) * MaxWidth);
			Pixels += BufferSize.X;
			Dest += MaxWidth;
		}

		// Generate the image on a background thread
		AsyncTask(ENamedThreads::AnyBackgroundHiPriTask, [this, Prompt, TargetSize, Size, InputStrength, Iterations, Seed, Framedata=CopiedFrame]()
		{
			FStableDiffusionImageResult result;
			result = this->GeneratorBridge->GenerateImageFromStartImage(Prompt, TargetSize.X, TargetSize.Y, Size.X, Size.Y, Framedata, TArray<FColor>(), InputStrength, Iterations, Seed);

			// Create generated texture on game thread
			AsyncTask(ENamedThreads::GameThread, [this, result]
			{
				this->OnImageGenerationComplete.Broadcast(result);
				this->OnImageGenerationCompleteEx.Broadcast(result);
				
				// Cleanup
				this->GeneratorBridge->OnImageProgressEx.RemoveDynamic(this, &UStableDiffusionSubsystem::UpdateImageProgress);
			});
		});

		// Don't need to keep capturing whilst generating
		ViewportCapture->StopCapturingFrames();
	});

	// Start frame capture
	ViewportCapture->CaptureThisFrame(framePtr);
}

bool UStableDiffusionSubsystem::SaveImageData(const FString& PackagePath, const FString& Name, UTexture2D* Texture)
{
	if (Name.IsEmpty() || PackagePath.IsEmpty() || !Texture)
		return false;

	// Create package
	FString FullPackagePath = PackagePath;
	FullPackagePath += Name;
	UPackage* Package = CreatePackage(NULL, *FullPackagePath);
	Package->FullyLoad();

	// Duplicate texture
	auto MipData = Texture->GetPlatformMips()[0].BulkData;
	UTexture2D* NewTexture = NewObject<UTexture2D>(Package, *Name, RF_Public | RF_Standalone | RF_MarkAsRootSet);
	NewTexture = ColorBufferToTexture(Name, static_cast<const uint8*>(MipData.LockReadOnly()), FIntPoint(Texture->GetSizeX(), Texture->GetSizeY()), NewTexture);
	MipData.Unlock();

	// Update package
	Package->MarkPackageDirty();
	FAssetRegistryModule::AssetCreated(NewTexture);
	FString PackageFileName = FPackageName::LongPackageNameToFilename(FullPackagePath, FPackageName::GetAssetPackageExtension());
	bool bSaved = UPackage::SavePackage(Package, NewTexture, EObjectFlags::RF_Public | EObjectFlags::RF_Standalone, *PackageFileName, GError, nullptr, true, true, SAVE_None);

	return bSaved;
}


void UStableDiffusionSubsystem::UpdateImageProgress(int32 Step, int32 Timestep, FIntPoint Size, const TArray<FColor>& PixelData)
{
	OnImageProgressUpdated.Broadcast(Step, Timestep, Size, PixelData);
}

UTexture2D* UStableDiffusionSubsystem::ColorBufferToTexture(const FString& FrameName, const TArray<FColor>& FrameColors, const FIntPoint& FrameSize, UTexture2D* OutTex)
{
	if (!FrameColors.Num())
		return nullptr;
	return ColorBufferToTexture(FrameName, (uint8*)FrameColors.GetData(), FrameSize, OutTex);
}

UTexture2D* UStableDiffusionSubsystem::ColorBufferToTexture(const FString& FrameName, const uint8* FrameData, const FIntPoint& FrameSize, UTexture2D* OutTex)
{
	if (!FrameData) 
		return nullptr;

	// Replace with CreateTexture2D from "ImageUtils.h"
	if (!OutTex) {
		TObjectPtr<UTexture2D> NewTex = NewObject<UTexture2D>();
		OutTex = NewTex;
	}

	OutTex->MipGenSettings = TMGS_NoMipmaps;
	OutTex->PlatformData = new FTexturePlatformData();
	OutTex->PlatformData->SizeX = FrameSize.X;
	OutTex->PlatformData->SizeY = FrameSize.Y;
	OutTex->PlatformData->SetNumSlices(1);
	OutTex->PlatformData->PixelFormat = EPixelFormat::PF_B8G8R8A8;

	// Allocate first mipmap.
	FTexture2DMipMap* Mip = nullptr;
	if (!OutTex->PlatformData->Mips.Num()) {
		Mip = new(OutTex->PlatformData->Mips) FTexture2DMipMap();
	}
	Mip->SizeX = FrameSize.X;
	Mip->SizeY = FrameSize.Y;

	// Lock the texture so it can be modified
	Mip->BulkData.Lock(LOCK_READ_WRITE);
	uint8* TextureData = (uint8*)Mip->BulkData.Realloc(FrameSize.X * FrameSize.Y * 4);
	FMemory::Memcpy(TextureData, FrameData, sizeof(uint8) * FrameSize.X * FrameSize.Y * 4);
	Mip->BulkData.Unlock();
	OutTex->UpdateResource();

	OutTex->Source.Init(FrameSize.X, FrameSize.Y, 1, 1, ETextureSourceFormat::TSF_RGBA8, reinterpret_cast<const uint8*>(FrameData));
	OutTex->UpdateResource();

	return OutTex;
}