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
#include "DesktopPlatformModule.h"

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

void UStableDiffusionSubsystem::InitModel(const FString& ModelName, const FString& Precision, const FString& Revision)
{
	if (GeneratorBridge) {
		AsyncTask(ENamedThreads::AnyBackgroundHiPriTask, [this, ModelName, Precision, Revision](){
			this->ModelInitialised = this->GeneratorBridge->InitModel(ModelName, Precision, Revision);
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

bool UStableDiffusionSubsystem::SaveTextureAsset(const FString& PackagePath, const FString& Name, UTexture2D* Texture)
{
	if (Name.IsEmpty() || PackagePath.IsEmpty() || !Texture)
		return false;

	// Create package
	FString FullPackagePath = FPaths::Combine(PackagePath, Name);
	UPackage* Package = CreatePackage(NULL, *FullPackagePath);
	Package->FullyLoad();

	// Duplicate texture
	auto SrcMipData = Texture->Source.LockMip(0);// GetPlatformMips()[0].BulkData;
	UTexture2D* NewTexture = NewObject<UTexture2D>(Package, *Name, RF_Public | RF_Standalone | RF_MarkAsRootSet);
	NewTexture->AddToRoot();
	NewTexture = ColorBufferToTexture(Name, SrcMipData, FIntPoint(Texture->GetSizeX(), Texture->GetSizeY()), NewTexture);
	Texture->Source.UnlockMip(0);

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

	if (!OutTex) {
		TObjectPtr<UTexture2D> NewTex = UTexture2D::CreateTransient(FrameSize.X, FrameSize.Y, EPixelFormat::PF_B8G8R8A8);
		OutTex = NewTex;
	}

	OutTex->Source.Init(FrameSize.X, FrameSize.Y, 1, 1, ETextureSourceFormat::TSF_BGRA8);//ETextureSourceFormat::TSF_RGBA8);
	OutTex->MipGenSettings = TMGS_NoMipmaps;
	OutTex->SRGB = true;
	OutTex->DeferCompression = true;

	uint8* TextureData = OutTex->Source.LockMip(0);
	FMemory::Memcpy(TextureData, FrameData, sizeof(uint8) * FrameSize.X * FrameSize.Y * 4);
	OutTex->Source.UnlockMip(0);
	OutTex->UpdateResource();

#if WITH_EDITOR
	OutTex->PostEditChange();
#endif
	return OutTex;
}

FString UStableDiffusionSubsystem::OpenImageFilePicker(const FString& StartDir)
{
	FString OpenFolderName;
	bool bOpen = false;

	IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();	
	if (DesktopPlatform)
	{
		FString ExtensionStr;
		ExtensionStr += TEXT("Image file (*.png)|*.exr|*.bmp|*.exr");
		bOpen = DesktopPlatform->OpenDirectoryDialog(
			FSlateApplication::Get().FindBestParentWindowHandleForDialogs(nullptr),
			"Save image to destination...",
			StartDir,
			OpenFolderName
		);
	}
	if (!bOpen)
	{
		return "";
	}

	return OpenFolderName;
}

FString UStableDiffusionSubsystem::FilepathToLongPackagePath(const FString& Path) 
{
	FString result;
	FString error;
	FPackageName::TryConvertFilenameToLongPackageName(Path, result, &error);
	return result;
}
