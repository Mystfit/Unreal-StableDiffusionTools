// Fill out your copyright notice in the Description page of Project Settings.
#include "StableDiffusionSubsystem.h"
#include "IAssetViewport.h"
#include "Engine/GameEngine.h"
#include "Async/Async.h"
#include "LevelEditor.h"
#include "IPythonScriptPlugin.h"
#include "PythonScriptTypes.h"
#include "LevelEditorSubsystem.h"
#include "ActorLayerUtilities.h"
#include "StableDiffusionImageResult.h"
#include "AssetRegistryModule.h"
#include "Components/SceneCaptureComponent2D.h"
#include "ImageUtils.h"
#include "EngineUtils.h"
#include "Dialogs/Dialogs.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/TextureRenderTarget2D.h"
#include "DesktopPlatformModule.h"

#define LOCTEXT_NAMESPACE "StableDiffusionSubsystem"

FString UStableDiffusionSubsystem::StencilLayerMaterialAsset = TEXT("/StableDiffusionTools/Materials/SD_StencilMask.SD_StencilMask");

bool FCapturedFramePayload::OnFrameReady_RenderThread(FColor* ColorBuffer, FIntPoint BufferSize, FIntPoint TargetSize) const
{
	OnFrameCapture.Broadcast(ColorBuffer, BufferSize, TargetSize);
	return true;
}

bool UStableDiffusionSubsystem::DependenciesAreInstalled()
{
	FPythonCommandEx PythonCommand;
	PythonCommand.Command = FString("SD_dependencies_installed()");
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


		// Present the user with a warning that changing projects has to restart the editor
		FSuppressableWarningDialog::FSetupInfo Info(
			LOCTEXT("RestartEditorMsg", "The editor will restart to complete the python dependency install process."), 
			LOCTEXT("RestartEditorTitle", "Restart editor"), "RestartEditorTitle_Warning");

		Info.ConfirmText = LOCTEXT("Yes", "Yes");
		Info.CancelText = LOCTEXT("No", "No");

		FSuppressableWarningDialog RestartDepsDlg(Info);
		bool bSwitch = true;
		if (RestartDepsDlg.ShowModal() == FSuppressableWarningDialog::Cancel)
		{
			bSwitch = false;
		}

		// If the user wants to continue with the restart set the pending project to swtich to and close the editor
		if (bSwitch)
		{
			// Close the editor.  This will prompt the user to save changes.  If they hit cancel, we abort the project switch
			//GEngine->DeferredCommands.Add(TEXT("CLOSE_SLATE_MAINFRAME"));
			FUnrealEdMisc::Get().RestartEditor(false);
		}
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

TSharedPtr<FSceneViewport> UStableDiffusionSubsystem::GetCapturingViewport()
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

	return OutSceneViewport;
}

void UStableDiffusionSubsystem::SetCaptureViewport(TSharedRef<FSceneViewport> Viewport, FIntPoint FrameSize)
{
	ViewportCapture = MakeShared<FFrameGrabber>(Viewport, FrameSize);
	ViewportCapture->StartCapturingFrames();
}

void UStableDiffusionSubsystem::GenerateImage(FStableDiffusionInput Input, bool FromViewport)
{
	AsyncTask(ENamedThreads::GameThread, [this, Input, FromViewport]() mutable
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

			auto ViewportSize = GetCapturingViewport()->GetSizeXY();

			// Create a SceneCapture2D that will capture our editor viewport
			CreateSceneCaptureCamera();

			// Forward image updated event from bridge to subsystem
			this->GeneratorBridge->OnImageProgressEx.AddUniqueDynamic(this, &UStableDiffusionSubsystem::UpdateImageProgress);

			// Restore screen messages and UI
			GAreScreenMessagesEnabled = bPrevGScreenMessagesEnabled;
			if (LevelEditorSubsystem)
				LevelEditorSubsystem->EditorSetGameView(bPrevViewportGameViewEnabled);

			// Setup scene capture component
			auto CaptureComponent = CurrentSceneCapture.SceneCapture->GetCaptureComponent2D();
			CaptureComponent->bCaptureEveryFrame = true;
			CaptureComponent->bCaptureOnMovement = false;
			CaptureComponent->CompositeMode = SCCM_Overwrite;
			CaptureComponent->CaptureSource = ESceneCaptureSource::SCS_FinalToneCurveHDR;

			// Create render target to hold our scene capture data
			UTextureRenderTarget2D* FullFrameRT = NewObject<UTextureRenderTarget2D>(CaptureComponent);
			check(FullFrameRT);
			FullFrameRT->InitCustomFormat(ViewportSize.X, ViewportSize.Y, PF_R8G8B8A8, false);
			FullFrameRT->UpdateResourceImmediate(true);
			CaptureComponent->TextureTarget = FullFrameRT;

			// Create destination pixel arrays
			TArray<FColor> FinalColor;
			TArray<FColor> InpaintMask;
			FinalColor.AddUninitialized(ViewportSize.X * ViewportSize.Y);
			InpaintMask.AddUninitialized(ViewportSize.X * ViewportSize.Y);
			FTextureRenderTargetResource* FullFrameRT_TexRes = FullFrameRT->GameThread_GetRenderTargetResource();

			// Capture scene and get main render pass
			CaptureComponent->CaptureScene();
			FullFrameRT_TexRes->ReadPixels(FinalColor, FReadSurfaceDataFlags());

			// Create material to handle actor layers as stencil masks
			TSoftObjectPtr<UMaterialInterface> StencilMatRef = TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(StencilLayerMaterialAsset));
			auto StencilLayerMaterial = StencilMatRef.LoadSynchronous();
			UMaterialInstanceDynamic* StencilMatInst = UMaterialInstanceDynamic::Create(StencilLayerMaterial, CaptureComponent);
			CaptureComponent->AddOrUpdateBlendable(StencilMatInst);
			
			// Disable bloom to stop it bleeding from the stencil mask
			CaptureComponent->ShowFlags.SetBloom(false);

			for (auto layer : Input.Options.InpaintLayers) {
				FScopedActorLayerStencil stencil_settings(layer);
				
				// Render second pass of scene as a stencil mask for inpainting
				CaptureComponent->CaptureScene();
				FullFrameRT_TexRes->ReadPixels(InpaintMask, FReadSurfaceDataFlags());

				// Cleanup scene capture once we've captured all our pixel data
				this->CurrentSceneCapture.SceneCapture->Destroy();
				this->CurrentSceneCapture.ViewportClient = nullptr;
			}

			// Copy frame data
			Input.InputImagePixels = MoveTempIfPossible(FinalColor);
			Input.MaskImagePixels = MoveTempIfPossible(InpaintMask);

			// Set size from viewport
			Input.Options.InSizeX = FromViewport ? ViewportSize.X : Input.Options.InSizeX;
			Input.Options.InSizeY = FromViewport ? ViewportSize.Y : Input.Options.InSizeY;

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
		});
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

void UStableDiffusionSubsystem::CreateSceneCaptureCamera()
{
	bool FoundViewport = false;
	FVector CameraLocation = FVector::ZeroVector;
	FRotator CameraRotation = FRotator::ZeroRotator;
	float HorizFov = 0.0f;

	for (FLevelEditorViewportClient* LevelVC : GEditor->GetLevelViewportClients())
	{
		if (LevelVC && LevelVC->IsPerspective())
		{
			FoundViewport = true;
			CurrentSceneCapture.ViewportClient = LevelVC;
			break;
		}
	}

	if (FoundViewport){
		CurrentSceneCapture.SceneCapture = GEditor->GetEditorWorldContext().World()->SpawnActor<ASceneCapture2D>();
		UpdateSceneCaptureCamera();
	}
}

void UStableDiffusionSubsystem::UpdateSceneCaptureCamera() 
{
	CurrentSceneCapture.SceneCapture->SetActorLocation(CurrentSceneCapture.ViewportClient->GetViewLocation());
	CurrentSceneCapture.SceneCapture->SetActorRotation(CurrentSceneCapture.ViewportClient->GetViewRotation());
	CurrentSceneCapture.SceneCapture->GetCaptureComponent2D()->FOVAngle = CurrentSceneCapture.ViewportClient->FOVAngle;
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

FScopedActorLayerStencil::FScopedActorLayerStencil(const FActorLayer& Layer)
{
	// Set custom depth variable to allow for stencil masks
	IConsoleVariable* CVar = IConsoleManager::Get().FindConsoleVariable(TEXT("r.CustomDepth"));
	if (CVar)
	{
		PreviousCustomDepthValue = CVar->GetInt();
		const int32 CustomDepthWithStencil = 3;
		if (PreviousCustomDepthValue != CustomDepthWithStencil)
		{
			UE_LOG(LogTemp, Log, TEXT("Overriding project custom depth/stencil value to support a stencil pass."));
			// We use ECVF_SetByProjectSetting otherwise once this is set once by rendering, the UI silently fails
			// if you try to change it afterwards. This SetByProjectSetting will fail if they have manipulated the cvar via the console
			// during their current session but it's less likely than changing the project settings.
			CVar->Set(CustomDepthWithStencil, EConsoleVariableFlags::ECVF_SetByProjectSetting);
		}
	}

	// If we're going to be using stencil layers, we need to cache all of the users
	// custom stencil/depth settings since we're changing them to do the mask.
	for (TActorIterator<AActor> ActorItr(GEditor->GetEditorWorldContext().World()); ActorItr; ++ActorItr)
	{
		AActor* Actor = *ActorItr;
		if (Actor)
		{
			for (UActorComponent* Component : Actor->GetComponents())
			{
				if (Component && Component->IsA<UPrimitiveComponent>())
				{
					UPrimitiveComponent* PrimitiveComponent = CastChecked<UPrimitiveComponent>(Component);
					FStencilValues& Values = ActorLayerSavedStencilValues.Add(PrimitiveComponent);
					Values.StencilMask = PrimitiveComponent->CustomDepthStencilWriteMask;
					Values.CustomStencil = PrimitiveComponent->CustomDepthStencilValue;
					Values.bRenderCustomDepth = PrimitiveComponent->bRenderCustomDepth;
				}
			}

			// The way stencil masking works is that we draw the actors on the given layer to the stencil buffer.
			// Then we apply a post-processing material which colors pixels outside those actors black, before
			// post processing. Then, TAA, Motion Blur, etc. is applied to all pixels. An alpha channel can preserve
			// which pixels were the geometry and which are dead space which lets you apply that as a mask later.
			bool bInLayer = true;

			// If this a normal layer, we only add the actor if it exists on this layer.
			bInLayer = Actor->Layers.Contains(Layer.Name);
			if (bInLayer) {
				UE_LOG(LogTemp, Log, TEXT("Actor found in stencil layer"));
			}

			for (UActorComponent* Component : Actor->GetComponents())
			{
				if (Component && Component->IsA<UPrimitiveComponent>())
				{
					UPrimitiveComponent* PrimitiveComponent = CastChecked<UPrimitiveComponent>(Component);
					// We want to render all objects not on the layer to stencil too so that foreground objects mask.
					PrimitiveComponent->SetCustomDepthStencilValue(bInLayer ? 1 : 0);
					PrimitiveComponent->SetCustomDepthStencilWriteMask(ERendererStencilMask::ERSM_Default);
					PrimitiveComponent->SetRenderCustomDepth(true);
				}
			}
		}
	}
}

FScopedActorLayerStencil::~FScopedActorLayerStencil()
{
	if (PreviousCustomDepthValue.IsSet())
	{
		IConsoleVariable* CVar = IConsoleManager::Get().FindConsoleVariable(TEXT("r.CustomDepth"));
		if (CVar)
		{
			if (CVar->GetInt() != PreviousCustomDepthValue.GetValue())
			{
				UE_LOG(LogTemp, Log, TEXT("Restoring custom depth/stencil value to: %d"), PreviousCustomDepthValue.GetValue());
				CVar->Set(PreviousCustomDepthValue.GetValue(), EConsoleVariableFlags::ECVF_SetByProjectSetting);
			}
		}
	}

	// Now we can restore the custom depth/stencil/etc. values so that the main render pass acts as the user expects next time.
	for (TPair<UPrimitiveComponent*, FStencilValues>& KVP : ActorLayerSavedStencilValues)
	{
		KVP.Key->SetCustomDepthStencilValue(KVP.Value.CustomStencil);
		KVP.Key->SetCustomDepthStencilWriteMask(KVP.Value.StencilMask);
		KVP.Key->SetRenderCustomDepth(KVP.Value.bRenderCustomDepth);
	}
}

#undef LOCTEXT_NAMESPACE
