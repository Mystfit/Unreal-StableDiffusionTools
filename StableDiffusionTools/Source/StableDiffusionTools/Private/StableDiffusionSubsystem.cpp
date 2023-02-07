// Fill out your copyright notice in the Description page of Project Settings.
#include "StableDiffusionSubsystem.h"
#include "IAssetViewport.h"
#include "Engine/GameEngine.h"
#include "Async/Async.h"
#include "Async/TaskGraphInterfaces.h"
#include "UObject/SavePackage.h"
#include "LevelEditor.h"
#include "IPythonScriptPlugin.h"
#include "PythonScriptTypes.h"
#include "LevelEditorSubsystem.h"
#include "ActorLayerUtilities.h"
#include "StableDiffusionImageResult.h"
#include "StableDiffusionToolsSettings.h"
#include "AssetRegistryModule.h"
#include "Components/SceneCaptureComponent2D.h"
#include "ImageUtils.h"
#include "MoviePipelineImageQuantization.h"
#include "ImagePixelData.h"
#include "EngineUtils.h"
#include "SLevelViewport.h"
#include "Dialogs/Dialogs.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/TextureRenderTarget2D.h"
#include "DesktopPlatformModule.h"

#define LOCTEXT_NAMESPACE "StableDiffusionSubsystem"

FString UStableDiffusionSubsystem::StencilLayerMaterialAsset = TEXT("/StableDiffusionTools/Materials/SD_StencilMask.SD_StencilMask");
FString UStableDiffusionSubsystem::DepthMaterialAsset = TEXT("/StableDiffusionTools/Materials/M_Depth.M_Depth");


bool FCapturedFramePayload::OnFrameReady_RenderThread(FColor* ColorBuffer, FIntPoint BufferSize, FIntPoint TargetSize) const
{
	OnFrameCapture.Broadcast(ColorBuffer, BufferSize, TargetSize);
	return true;
}

UStableDiffusionSubsystem::UStableDiffusionSubsystem(const FObjectInitializer& initializer)
{
	// Wait for Python to load our derived classes before we construct the bridge
	IPythonScriptPlugin& PythonModule = FModuleManager::LoadModuleChecked<IPythonScriptPlugin>(TEXT("PythonScriptPlugin"));
	PythonModule.OnPythonInitialized().AddLambda([this]() {
		// Make sure that we have our Python derived bridges available in the settings first
		GetMutableDefault<UStableDiffusionToolsSettings>()->ReloadConfig(UStableDiffusionToolsSettings::StaticClass());
		auto BridgeClass = GetDefault<UStableDiffusionToolsSettings>()->GetGeneratorType();
		this->CreateBridge(BridgeClass);
	}); 
	//PythonModule.OnPythonInitialized().AddUFunction(this, GET_FUNCTION_NAME_CHECKED(UStableDiffusionSubsystem, CreateBridge));
}

bool UStableDiffusionSubsystem::IsBridgeLoaded()
{
	return (GeneratorBridge == nullptr) ? false : true;
}

void UStableDiffusionSubsystem::CreateBridge(TSubclassOf<UStableDiffusionBridge> BridgeClass)
{
	auto BaseClass = UStableDiffusionBridge::StaticClass();
	if (BridgeClass == BaseClass) {
		UE_LOG(LogTemp, Warning, TEXT("Can not create Stable Diffusion Bridge. Only classes deriving from %s can be created."), *BridgeClass->GetName())
			return;
	}

	TArray<UClass*> PythonBridgeClasses;
	GetDerivedClasses(UStableDiffusionBridge::StaticClass(), PythonBridgeClasses);

	for (auto DerivedBridgeClass : PythonBridgeClasses) {
		if (DerivedBridgeClass->IsChildOf(BridgeClass)) {
				
			// We need to create the bridge class from inside Python so that python created objects don't get GC'd
			FPythonCommandEx PythonCommand;
			PythonCommand.Command = FString::Printf(
				TEXT("from bridges import %s; "\
				"bridge = %s.%s(); "\
				"subsystem = unreal.get_editor_subsystem(unreal.StableDiffusionSubsystem); "\
				"subsystem.set_editor_property('GeneratorBridge', bridge)"), 
				*DerivedBridgeClass->GetName(),
				*DerivedBridgeClass->GetName(),
				*DerivedBridgeClass->GetName()
			);
			PythonCommand.ExecutionMode = EPythonCommandExecutionMode::ExecuteStatement;
			PythonCommand.FileExecutionScope = EPythonFileExecutionScope::Public;
			bool Result = IPythonScriptPlugin::Get()->ExecPythonCommandEx(PythonCommand);
			if (!Result) {
				UE_LOG(LogTemp, Error, TEXT("Failed to load SD bridge %s"), *DerivedBridgeClass->GetName())
			}

			break;
		}
	}

	//GeneratorBridge = NewObject<UStableDiffusionBridge>(this, FName(*BridgeClass->GetName()), RF_Public | RF_Standalone, BridgeClass->ClassDefaultObject);
	if (GeneratorBridge) {
		//GeneratorBridge->AddToRoot();
		OnBridgeLoadedEx.Broadcast(GeneratorBridge);
	}
	
}

bool UStableDiffusionSubsystem::DependenciesAreInstalled()
{
	return (DependencyManager) ? DependencyManager->AllDependenciesInstalled() : false;
}

void UStableDiffusionSubsystem::RestartEditor()
{
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
		FUnrealEdMisc::Get().RestartEditor(false);
	}
}

void UStableDiffusionSubsystem::InstallDependency(FDependencyManifestEntry Dependency, bool ForceReinstall)
{
	AsyncTask(ENamedThreads::AnyBackgroundHiPriTask, [this, Dependency, ForceReinstall]() {
		if (this->DependencyManager) {
			FDependencyStatus result = this->DependencyManager->InstallDependency(Dependency, ForceReinstall);

			AsyncTask(ENamedThreads::GameThread, [this, result]() {
				this->DependencyManager->OnDependencyInstalled.Broadcast(result);
			});
		}
	});
}

bool UStableDiffusionSubsystem::HasToken()
{
	if (GeneratorBridge) {
		return !GeneratorBridge->GetToken().IsEmpty();
	}
	return false;
}

FString UStableDiffusionSubsystem::GetToken()
{
	if (GeneratorBridge) {
		return GeneratorBridge->GetToken();
	}
	return "";
}

bool UStableDiffusionSubsystem::LoginUsingToken(const FString& token)
{
	if (GeneratorBridge) {
		return GeneratorBridge->LoginUsingToken(token);
	}
	return false;
}

void UStableDiffusionSubsystem::InitModel(const FStableDiffusionModelOptions& Model, bool Async)
{
	if (GeneratorBridge) {
		// Unload any loaded models first
		//ReleaseModel();

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
	if (GeneratorBridge) {
		GeneratorBridge->ReleaseModel();
		ModelInitialised = false;
	}
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

void UStableDiffusionSubsystem::StartCapturingViewport()
{
	// Find active viewport
	TSharedPtr<FSceneViewport> OutSceneViewport = GetCapturingViewport();
	SetCaptureViewport(OutSceneViewport.ToSharedRef(), OutSceneViewport->GetSize());
}

void UStableDiffusionSubsystem::SetCaptureViewport(TSharedRef<FSceneViewport> Viewport, FIntPoint FrameSize)
{
	ViewportCapture = MakeShared<FFrameGrabber>(Viewport, FrameSize);
	ViewportCapture->StartCapturingFrames();
}

void UStableDiffusionSubsystem::GenerateImage(FStableDiffusionInput Input, EInputImageSource ImageSourceType)
{
	if (!GeneratorBridge)
		return;

	bIsGenerating = true;

	AsyncTask(ENamedThreads::GameThread, [this, Input, ImageSourceType]() mutable
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
			if (ImageSourceType == EInputImageSource::Viewport) {
				CaptureFromViewportSource(MoveTempIfPossible(Input));
			}
			else if (ImageSourceType == EInputImageSource::SceneCapture2D) {
				CaptureFromSceneCaptureSource(MoveTempIfPossible(Input));
			}

			// Restore screen messages and UI
			GAreScreenMessagesEnabled = bPrevGScreenMessagesEnabled;
			if (LevelEditorSubsystem)
				LevelEditorSubsystem->EditorSetGameView(bPrevViewportGameViewEnabled);
		});
}

void UStableDiffusionSubsystem::StartImageGeneration(FStableDiffusionInput Input)
{
	// Generate the image on a background thread
	//CurrentRenderTask = TGraphTask<FSDRenderTask>::CreateTask().ConstructAndDispatchWhenReady(ENamedThreads::AnyBackgroundHiPriTask, MoveTemp([this, Input]()
	AsyncTask(ENamedThreads::AnyBackgroundHiPriTask, [this, Input]()
		{
			// Forward image updated event from bridge to subsystem
			this->GeneratorBridge->OnImageProgressEx.AddUniqueDynamic(this, &UStableDiffusionSubsystem::UpdateImageProgress);

			// Generate image
			FStableDiffusionImageResult result = this->GeneratorBridge->GenerateImageFromStartImage(Input);
			

			// Create generated texture on game thread
			AsyncTask(ENamedThreads::GameThread, [this, result]
			{
				bIsGenerating = false;
				this->OnImageGenerationCompleteEx.Broadcast(result);

				// Cleanup
				this->GeneratorBridge->OnImageProgressEx.RemoveDynamic(this, &UStableDiffusionSubsystem::UpdateImageProgress);
			});
	});
	//);
}

void UStableDiffusionSubsystem::UpsampleImage(const FStableDiffusionImageResult& input)
{
	if (!GeneratorBridge)
		return;

	AsyncTask(ENamedThreads::AnyBackgroundHiPriTask, [this, input](){
		auto result = GeneratorBridge->UpsampleImage(input);
		AsyncTask(ENamedThreads::GameThread, [this, result=MoveTemp(result)]() {
			OnImageUpsampleCompleteEx.Broadcast(result);
		});
	});
}

bool UStableDiffusionSubsystem::SaveTextureAsset(const FString& PackagePath, const FString& Name, UTexture2D* Texture, const FStableDiffusionGenerationOptions& ImageInputs, bool Upsampled)
{
	if (Name.IsEmpty() || PackagePath.IsEmpty() || !Texture)
		return false;

	// Create package
	FString FullPackagePath = FPaths::Combine(PackagePath, Name);
	UPackage* Package = CreatePackage(this, *FullPackagePath);
	Package->FullyLoad();

	// Duplicate texture
	auto SrcMipData = Texture->Source.LockMip(0);// GetPlatformMips()[0].BulkData;
	FString TexName = "T_" + Name;
	UTexture2D* NewTexture = NewObject<UTexture2D>(Package, *TexName, RF_Public | RF_Standalone | RF_MarkAsRootSet);
	NewTexture->AddToRoot();
	NewTexture = ColorBufferToTexture(Name, SrcMipData, FIntPoint(Texture->GetSizeX(), Texture->GetSizeY()), NewTexture);
	Texture->Source.UnlockMip(0);

	// Create data asset
	FString AssetName = "DA_" + Name;
	UStableDiffusionImageResultAsset* NewImageResultAsset = NewObject<UStableDiffusionImageResultAsset>(Package, *AssetName, RF_Public | RF_Standalone | RF_MarkAsRootSet);
	NewImageResultAsset->ImageInputs = ImageInputs;
	NewImageResultAsset->Upsampled = Upsampled;
	NewImageResultAsset->ImageOutput = NewTexture;

	// Update package
	Package->MarkPackageDirty();
	FAssetRegistryModule::AssetCreated(NewTexture);
	
	// Save texture pacakge
	FString PackageFileName = FPackageName::LongPackageNameToFilename(FullPackagePath, FPackageName::GetAssetPackageExtension());
	FSavePackageArgs PackageArgs;
	PackageArgs.TopLevelFlags = EObjectFlags::RF_Public | EObjectFlags::RF_Standalone;
	PackageArgs.bForceByteSwapping = true;
	bool bSaved = UPackage::SavePackage(Package, NewTexture, *PackageFileName, PackageArgs);

	return bSaved;
}

void UStableDiffusionSubsystem::UpdateImageProgress(int32 Step, int32 Timestep, FIntPoint Size, const TArray<FColor>& PixelData)
{
	OnImageProgressUpdated.Broadcast(Step, Timestep, Size, PixelData);
}

void UStableDiffusionSubsystem::SetLivePreviewEnabled(bool Enabled, float Delay)
{
	if (Enabled && !OnEditorCameraUpdatedDlgHandle.IsValid()) {
		OnEditorCameraUpdatedDlgHandle = FEditorDelegates::OnEditorCameraMoved.AddLambda([this, Delay](const FVector& Location, const FRotator& Rotation, ELevelViewportType ViewportType, int32 ViewportIndex) {
			UE_LOG(LogTemp, Log, TEXT("Moving editor camera to %s %s"), *Location.ToString(), *Rotation.ToString())

			FEditorCameraLivePreview CameraInfo;
			CameraInfo.Location = Location;
			CameraInfo.Rotation = Rotation;
			CameraInfo.ViewportType = ViewportType;
			CameraInfo.ViewportIndex = ViewportIndex;

			// Only broadcast when the camera is not moving
			if (!(LastPreviewCameraInfo == CameraInfo)) {
				GEditor->GetTimerManager()->SetTimer(IdleCameraTimer, this, &UStableDiffusionSubsystem::LivePreviewUpdate, Delay, false);
			}

			LastPreviewCameraInfo = CameraInfo;
		});
	}
	else if (!Enabled && OnEditorCameraUpdatedDlgHandle.IsValid()) {
		FEditorDelegates::OnEditorCameraMoved.Remove(OnEditorCameraUpdatedDlgHandle);
		OnEditorCameraUpdatedDlgHandle.Reset();
	}
	
	/*FEditorDelegates::RefreshEditor.AddLambda([]() {
		UE_LOG(LogTemp, Log, TEXT("Editor refreshed"));
	});*/
}

UTexture2D* UStableDiffusionSubsystem::ColorBufferToTexture(const FString& FrameName, const TArray<FColor>& FrameColors, const FIntPoint& FrameSize, UTexture2D* OutTex)
{
	if (!FrameColors.Num())
		return nullptr;
	return ColorBufferToTexture(FrameName, (uint8*)FrameColors.GetData(), FrameSize, OutTex);
}

FViewportSceneCapture UStableDiffusionSubsystem::CreateSceneCaptureCamera()
{
	FViewportSceneCapture SceneCapture;

	bool FoundViewport = false;
	FVector CameraLocation = FVector::ZeroVector;
	FRotator CameraRotation = FRotator::ZeroRotator;
	float HorizFov = 0.0f;

	for (FLevelEditorViewportClient* LevelVC : GEditor->GetLevelViewportClients())
	{
		if (LevelVC && LevelVC->IsPerspective())
		{
			FoundViewport = true;
			SceneCapture.ViewportClient = LevelVC;
			break;
		}
	}

	if (FoundViewport){
		SceneCapture.SceneCapture = GEditor->GetEditorWorldContext().World()->SpawnActor<ASceneCapture2D>();
		UpdateSceneCaptureCamera(SceneCapture);
	}

	return SceneCapture;
}

void UStableDiffusionSubsystem::UpdateSceneCaptureCamera(FViewportSceneCapture& SceneCapture)
{
	SceneCapture.SceneCapture->SetActorLocation(SceneCapture.ViewportClient->GetViewLocation());
	SceneCapture.SceneCapture->SetActorRotation(SceneCapture.ViewportClient->GetViewRotation());
	
	auto CaptureComponent = SceneCapture.SceneCapture->GetCaptureComponent2D();
	CaptureComponent->FOVAngle = SceneCapture.ViewportClient->FOVAngle;
	CaptureComponent->bCaptureEveryFrame = true;
	CaptureComponent->bCaptureOnMovement = false;
	CaptureComponent->CompositeMode = SCCM_Overwrite;
	CaptureComponent->CaptureSource = ESceneCaptureSource::SCS_FinalToneCurveHDR;
	//CaptureComponent->ShowFlags.SetBloom(false);
}

void UStableDiffusionSubsystem::CaptureFromViewportSource(FStableDiffusionInput Input)
{
	auto ViewportSize = GetCapturingViewport()->GetSizeXY();

	// Make sure viewport capture objects are available
	StartCapturingViewport();

	// Capture stencil layer for inpaint models
	if ((ModelOptions.Capabilities & (int32)EModelCapabilities::INPAINT) == (int32)EModelCapabilities::INPAINT) {
		if (Input.InpaintLayers.Num()) {
			auto SceneCapture = CreateSceneCaptureCamera();
			Input.MaskImagePixels = CaptureStencilMask(SceneCapture.SceneCapture->GetCaptureComponent2D(), ViewportSize, Input.InpaintLayers[0]);
			SceneCapture.SceneCapture->ConditionalBeginDestroy();
		}
	}

	// Capture depth map layer for depth models
	if ((ModelOptions.Capabilities & (int32)EModelCapabilities::DEPTH) == (int32)EModelCapabilities::DEPTH) {
		auto SceneCapture = CreateSceneCaptureCamera();
		Input.MaskImagePixels = CaptureDepthMap(SceneCapture.SceneCapture->GetCaptureComponent2D(), ViewportSize, Input.SceneDepthScale, Input.SceneDepthOffset);
		SceneCapture.SceneCapture->ConditionalBeginDestroy();
	}

	// Create a frame payload we will wait on to be filled with a frame
	auto framePtr = MakeShared<FCapturedFramePayload>();
	framePtr->OnFrameCapture.AddLambda([=](FColor* Pixels, FIntPoint BufferSize, FIntPoint TargetSize) mutable {
		// Copy frame data
		TArray<FColor> CopiedFrame = CopyFrameData(TargetSize, BufferSize, Pixels);
	Input.InputImagePixels = MoveTempIfPossible(CopiedFrame);

	// Don't need to keep capturing whilst generating
	ViewportCapture->StopCapturingFrames();

	// Set size from viewport
	Input.Options.InSizeX = ViewportSize.X;
	Input.Options.InSizeY = ViewportSize.Y;

	// Only start image generation when we have a frame
	StartImageGeneration(Input);
		});

	// Start frame capture
	ViewportCapture->CaptureThisFrame(framePtr);
}

void UStableDiffusionSubsystem::CaptureFromSceneCaptureSource(FStableDiffusionInput Input)
{
	// Use chosen scene capture component or create a default one
	USceneCaptureComponent2D* CaptureComponent = nullptr;
	if (!Input.CaptureSource) {
		// Create a default SceneCapture2D that will capture our editor viewport
		CurrentSceneCapture = CreateSceneCaptureCamera();
		CaptureComponent = CurrentSceneCapture.SceneCapture->GetCaptureComponent2D();
	}
	else {
		CaptureComponent = Input.CaptureSource;
	}

	// Get the capture size from the source
	auto ViewportSize = GetCapturingViewport()->GetSizeXY();
	FIntPoint CaptureSize = (CaptureComponent->TextureTarget) ? FIntPoint(CaptureComponent->TextureTarget->SizeX, CaptureComponent->TextureTarget->SizeY) : ViewportSize;

	// Remember original properties we're overriding in the capture component
	auto LastCaptureEveryFrame = CaptureComponent->bCaptureEveryFrame;
	auto LastCaptureOnMovement = CaptureComponent->bCaptureOnMovement;
	auto LastCompositeMode = CaptureComponent->CompositeMode;
	auto LastCaptureSource = CaptureComponent->CaptureSource;
	auto LastTextureTarget = CaptureComponent->TextureTarget;
	auto LastBloom = CaptureComponent->ShowFlags.Bloom;
	auto LastBlendables = CaptureComponent->PostProcessSettings.WeightedBlendables;

	// Set capture overrides
	CaptureComponent->bCaptureEveryFrame = true;
	CaptureComponent->bCaptureOnMovement = false;
	CaptureComponent->CompositeMode = SCCM_Overwrite;
	CaptureComponent->CaptureSource = ESceneCaptureSource::SCS_FinalToneCurveHDR;
	CaptureComponent->ShowFlags.SetBloom(false);
	//CaptureComponent->PostProcessSettings.AutoExposureMethod = EAutoExposureMethod::AEM_Manual;
	//CaptureComponent->PostProcessSettings.AutoExposureBias = 15;

	// Create render target to hold our scene capture data
	UTextureRenderTarget2D* FullFrameRT = NewObject<UTextureRenderTarget2D>(CaptureComponent);
	check(FullFrameRT);
	FullFrameRT->InitCustomFormat(CaptureSize.X, CaptureSize.Y, PF_R8G8B8A8, false);
	FullFrameRT->UpdateResourceImmediate(true);
	CaptureComponent->TextureTarget = FullFrameRT;

	// Create destination pixel arrays
	TArray<FColor> FinalColor;
	TArray<FColor> InpaintMask;
	FinalColor.AddUninitialized(CaptureSize.X * CaptureSize.Y);
	InpaintMask.AddUninitialized(CaptureSize.X * CaptureSize.Y);
	FTextureRenderTargetResource* FullFrameRT_TexRes = FullFrameRT->GameThread_GetRenderTargetResource();

	// Capture scene and get main render pass
	CaptureComponent->CaptureScene();
	FullFrameRT_TexRes->ReadPixels(FinalColor, FReadSurfaceDataFlags());

	// Copy frame data
	// Capture stencil layer for inpaint models
	if ((ModelOptions.Capabilities & (int32)EModelCapabilities::INPAINT) == (int32)EModelCapabilities::INPAINT) {
		if (Input.InpaintLayers.Num())
			Input.MaskImagePixels = CaptureStencilMask(CaptureComponent, CaptureSize, Input.InpaintLayers[0]);
	}

	// Capture depth map layer for depth models
	if ((ModelOptions.Capabilities & (int32)EModelCapabilities::DEPTH) == (int32)EModelCapabilities::DEPTH) {
		Input.MaskImagePixels = CaptureDepthMap(CaptureComponent, CaptureSize, Input.SceneDepthScale, Input.SceneDepthOffset);
	}
	Input.InputImagePixels = MoveTempIfPossible(FinalColor);

	// Set size from scene capture
	Input.Options.InSizeX = CaptureSize.X;
	Input.Options.InSizeY = CaptureSize.Y;

	if (!Input.CaptureSource && this->CurrentSceneCapture.SceneCapture) {
		// Cleanup created scene capture once we've captured all our pixel data
		this->CurrentSceneCapture.SceneCapture->ConditionalBeginDestroy();
		this->CurrentSceneCapture.ViewportClient = nullptr;
	}
	else {
		// Restore original capture component properties
		CaptureComponent->bCaptureEveryFrame = LastCaptureEveryFrame;
		CaptureComponent->bCaptureOnMovement = LastCaptureOnMovement;
		CaptureComponent->CompositeMode = LastCompositeMode;
		CaptureComponent->CaptureSource = LastCaptureSource;
		CaptureComponent->TextureTarget = LastTextureTarget;
		CaptureComponent->ShowFlags.Bloom = LastBloom;
		CaptureComponent->PostProcessSettings.WeightedBlendables = LastBlendables;
	}

	StartImageGeneration(Input);
}

TArray<FColor> UStableDiffusionSubsystem::CaptureDepthMap(USceneCaptureComponent2D* CaptureSource, FIntPoint Size, float SceneDepthScale, float SceneDepthOffset)
{
	check(CaptureSource);

	// Assign pixel arrays
	TArray<FColor> DepthPixels;
	DepthPixels.AddUninitialized(Size.X * Size.Y);
	
	// Create material to render depth postprocess mat
	TSoftObjectPtr<UMaterialInterface> DepthMatRef = TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(DepthMaterialAsset));
	auto DepthMaterial = DepthMatRef.LoadSynchronous();
	UMaterialInstanceDynamic* DepthMatInst = UMaterialInstanceDynamic::Create(DepthMaterial, CaptureSource);
	DepthMatInst->SetScalarParameterValue("DepthScale", SceneDepthScale);
	DepthMatInst->SetScalarParameterValue("StartDepth", SceneDepthOffset);
	CaptureSource->AddOrUpdateBlendable(DepthMatInst);

	// Create depth render target
	UTextureRenderTarget2D* FullFrameDepthRT = NewObject<UTextureRenderTarget2D>();
	check(FullFrameDepthRT);
	FullFrameDepthRT->InitCustomFormat(Size.X, Size.Y, PF_FloatRGBA, true);
	FullFrameDepthRT->UpdateResourceImmediate(true);
	FTextureRenderTargetResource* FullFrameDepthRT_TexRes = FullFrameDepthRT->GameThread_GetRenderTargetResource();

	// Render second pass containing scene depth
	CaptureSource->TextureTarget = FullFrameDepthRT;
	CaptureSource->CaptureSource = ESceneCaptureSource::SCS_FinalColorHDR;
	CaptureSource->CaptureScene();

	// Read depthmap and convert to 8bit
	TArray<FLinearColor> DepthMap32bit;
	FullFrameDepthRT_TexRes->ReadLinearColorPixels(DepthMap32bit);
	for (size_t idx = 0; idx < DepthMap32bit.Num(); ++idx) {
		int NormalizedDepth = DepthMap32bit[idx].R * 255;
		DepthPixels[idx] = FColor(NormalizedDepth, NormalizedDepth, NormalizedDepth, 255);
	}

	// Cleanup
	//FullFrameDepthRT->ConditionalBeginDestroy();
	//FullFrameDepthRT = nullptr;
	//FullFrameDepthRT_TexRes = nullptr;

	//DepthMatInst->ConditionalBeginDestroy();
	//DepthMatInst = nullptr;

	return MoveTemp(DepthPixels);
}

TArray<FColor> UStableDiffusionSubsystem::CaptureStencilMask(USceneCaptureComponent2D* CaptureSource, FIntPoint Size, FActorLayer Layer)
{
	check(CaptureSource);

	// Assign pixel arrays
	TArray<FColor> InpaintMaskPixels;
	InpaintMaskPixels.AddUninitialized(Size.X * Size.Y);

	// Create render target to hold our scene capture data
	UTextureRenderTarget2D* FullFrameRT = NewObject<UTextureRenderTarget2D>(CaptureSource);
	check(FullFrameRT);
	FullFrameRT->InitCustomFormat(Size.X, Size.Y, PF_R8G8B8A8, false);
	FullFrameRT->UpdateResourceImmediate(true);
	CaptureSource->TextureTarget = FullFrameRT;
	FTextureRenderTargetResource* FullFrameRT_TexRes = FullFrameRT->GameThread_GetRenderTargetResource();

	// Create material to handle actor layers as stencil masks
	TSoftObjectPtr<UMaterialInterface> StencilMatRef = TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(StencilLayerMaterialAsset));
	auto StencilLayerMaterial = StencilMatRef.LoadSynchronous();
	UMaterialInstanceDynamic* StencilMatInst = UMaterialInstanceDynamic::Create(StencilLayerMaterial, CaptureSource);
	CaptureSource->AddOrUpdateBlendable(StencilMatInst);

	// Set scene stencil settings
	FScopedActorLayerStencil stencil_settings(Layer);

	// Render pass of scene as a stencil mask for inpainting
	CaptureSource->CaptureScene();
	FullFrameRT_TexRes->ReadPixels(InpaintMaskPixels, FReadSurfaceDataFlags());

	// Cleanup render target and resources
	//FullFrameRT->ConditionalBeginDestroy();
	//FullFrameRT = nullptr;
	//FullFrameRT_TexRes = nullptr;

	//StencilMatInst->ConditionalBeginDestroy();
	//StencilMatInst = nullptr;

	return MoveTemp(InpaintMaskPixels);
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

void UStableDiffusionSubsystem::LivePreviewUpdate()
{
	OnEditorCameraMovedEx.Broadcast(LastPreviewCameraInfo);
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

FScopedActorLayerStencil::FScopedActorLayerStencil(const FActorLayer& Layer, bool RestoreOnDelete) : RestoreOnDelete(RestoreOnDelete)
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

FScopedActorLayerStencil::FScopedActorLayerStencil(const FScopedActorLayerStencil& ref) {
	ActorLayerSavedStencilValues = ref.ActorLayerSavedStencilValues;
	RestoreOnDelete = ref.RestoreOnDelete;
	PreviousCustomDepthValue = ref.PreviousCustomDepthValue;
}

FScopedActorLayerStencil::~FScopedActorLayerStencil()
{
	if (RestoreOnDelete)
		Restore();
}

void FScopedActorLayerStencil::Restore() {
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
