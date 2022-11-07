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
		FPythonCommandEx DepInstallCommand;
		DepInstallCommand.Command = FString("install_dependencies.py");
		DepInstallCommand.ExecutionMode = EPythonCommandExecutionMode::ExecuteFile;
		DepInstallCommand.FileExecutionScope = EPythonFileExecutionScope::Public;
		
		bool install_result = IPythonScriptPlugin::Get()->ExecPythonCommandEx(DepInstallCommand);
		if (!install_result) {
			// Dependency installation failed - log result
			OnDependenciesInstalled.Broadcast(install_result);
			return;
		}

		FPythonCommandEx LoadBridgeCommand;
		LoadBridgeCommand.Command = FString("load_diffusers_bridge.py");
		LoadBridgeCommand.ExecutionMode = EPythonCommandExecutionMode::ExecuteFile;
		LoadBridgeCommand.FileExecutionScope = EPythonFileExecutionScope::Public;
		bool reload_result = IPythonScriptPlugin::Get()->ExecPythonCommandEx(LoadBridgeCommand);
		OnDependenciesInstalled.Broadcast(reload_result);
}

bool UStableDiffusionSubsystem::HasHuggingFaceToken()
{
	auto token = GetHuggingfaceToken();
	return token != "None" && !token.IsEmpty();
}

FString UStableDiffusionSubsystem::GetHuggingfaceToken()
{
	FPythonCommandEx PythonCommand;
	PythonCommand.Command = FString("import huggingface_hub");
	PythonCommand.ExecutionMode = EPythonCommandExecutionMode::ExecuteStatement;
	PythonCommand.FileExecutionScope = EPythonFileExecutionScope::Public;
	IPythonScriptPlugin::Get()->ExecPythonCommandEx(PythonCommand);

	PythonCommand.Command = FString("huggingface_hub.utils.HfFolder.get_token()");
	PythonCommand.ExecutionMode = EPythonCommandExecutionMode::EvaluateStatement;
	IPythonScriptPlugin::Get()->ExecPythonCommandEx(PythonCommand);

	//Python evaluation is wrapped in single quotes
	bool trimmed = false;
	auto result = PythonCommand.CommandResult.TrimChar(TCHAR('\''), &trimmed).TrimQuotes().TrimEnd();
	return result;
}

bool UStableDiffusionSubsystem::LoginHuggingFaceUsingToken(const FString& token)
{
	FPythonCommandEx PythonCommand;
	PythonCommand.Command = FString::Format(TEXT("huggingface_hub.utils.HfFolder.save_token('{0}')"), { token });
	PythonCommand.ExecutionMode = EPythonCommandExecutionMode::ExecuteStatement;
	return IPythonScriptPlugin::Get()->ExecPythonCommandEx(PythonCommand);
}

void UStableDiffusionSubsystem::InitModel(const FStableDiffusionModelOptions& Model, bool Async)
{
	if (GeneratorBridge) {
		if (Async) {
			AsyncTask(ENamedThreads::AnyBackgroundHiPriTask, [this, Model]() {
				this->ModelInitialised = this->GeneratorBridge->InitModel(Model);
				if (this->ModelInitialised)
					ModelOptions = Model;

				AsyncTask(ENamedThreads::GameThread, [this]() {
					this->OnModelInitializedEx.Broadcast(this->ModelInitialised);
					});
				});
		}
		else {
			this->ModelInitialised = this->GeneratorBridge->InitModel(Model);
			if (this->ModelInitialised)
				ModelOptions = Model;

			AsyncTask(ENamedThreads::GameThread, [this]() {
				this->OnModelInitializedEx.Broadcast(this->ModelInitialised);
			});
		}
	}
}

void UStableDiffusionSubsystem::ReleaseModel()
{
	GeneratorBridge->ReleaseModel();
	ModelInitialised = false;
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

void UStableDiffusionSubsystem::GenerateImage(FStableDiffusionInput Input, bool FromViewport)
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

	StartCapturingViewport(FIntPoint(Input.Options.OutSizeX, Input.Options.OutSizeY));

	if (!ViewportCapture || ActiveEndframeHandler.IsValid())
		return;

	// Forward image updated event from bridge to subsystem
	this->GeneratorBridge->OnImageProgressEx.AddUniqueDynamic(this, &UStableDiffusionSubsystem::UpdateImageProgress);

	// Create a frame payload we will wait on to be filled with a frame
	auto framePtr = MakeShared<FCapturedFramePayload>();
	framePtr->OnFrameCapture.AddLambda([=](FColor* Pixels, FIntPoint BufferSize, FIntPoint TargetSize) mutable
	{
		// Restore screen messages and UI
		GAreScreenMessagesEnabled = bPrevGScreenMessagesEnabled;
		if(LevelEditorSubsystem)
			LevelEditorSubsystem->EditorSetGameView(bPrevViewportGameViewEnabled);

		// Copy frame data
		TArray<FColor> CopiedFrame = CopyFrameData(TargetSize, BufferSize, Pixels);
		Input.InputImagePixels = MoveTempIfPossible(CopiedFrame);

		// Set size from viewport
		Input.Options.InSizeX = FromViewport ? TargetSize.X : Input.Options.InSizeX;
		Input.Options.InSizeY = FromViewport ? TargetSize.Y : Input.Options.InSizeY;

		// Generate the image on a background thread
		AsyncTask(ENamedThreads::AnyBackgroundHiPriTask, [this, Input]()
		{
			// Generate image
			FStableDiffusionImageResult result = this->GeneratorBridge->GenerateImageFromStartImage(Input);

			// Create generated texture on game thread
			AsyncTask(ENamedThreads::GameThread, [this, result]
			{
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

void UStableDiffusionSubsystem::UpsampleImage(const FStableDiffusionImageResult& input)
{
	AsyncTask(ENamedThreads::AnyBackgroundHiPriTask, [this, input](){
		auto result = GeneratorBridge->UpsampleImage(input);
		AsyncTask(ENamedThreads::GameThread, [this, result=MoveTemp(result)]() {
			OnImageUpsampleCompleteEx.Broadcast(result);
		});
	});
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


TArray<FColor> UStableDiffusionSubsystem::CopyFrameData(FIntPoint TargetSize, FIntPoint BufferSize, FColor* ColorBuffer)
{
	// Copy frame data
	TArray<FColor> CopiedFrame;

	CopiedFrame.InsertUninitialized(0, TargetSize.X * TargetSize.Y);
	FColor* Dest = &CopiedFrame[0];
	const int32 MaxWidth = FMath::Min(TargetSize.X, BufferSize.X);
	for (int32 Row = 0; Row < FMath::Min(TargetSize.Y, BufferSize.Y); ++Row)
	{
		FMemory::Memcpy(Dest, ColorBuffer, sizeof(FColor) * MaxWidth);
		ColorBuffer += BufferSize.X;
		Dest += MaxWidth;
	}

	return std::move(CopiedFrame);
}
