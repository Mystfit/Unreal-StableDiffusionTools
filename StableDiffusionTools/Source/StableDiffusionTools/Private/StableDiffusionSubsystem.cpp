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
#include "Engine/TextureRenderTarget2D.h"
#include "DesktopPlatformModule.h"
#include "StableDiffusionBlueprintLibrary.h"
#include "LayerProcessors/FinalColorLayerProcessor.h"

#define LOCTEXT_NAMESPACE "StableDiffusionSubsystem"

FString UStableDiffusionSubsystem::NormalMaterialAsset = TEXT("/StableDiffusionTools/Materials/M_Normals.M_Normals");


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

		// Set Python loaded flags and events
		PythonLoaded = true;
		OnPythonLoadedEx.Broadcast();
		OnPythonLoaded.Broadcast();
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

bool UStableDiffusionSubsystem::DependenciesAreInstalled() const
{
	return (DependencyManager) ? DependencyManager->AllDependenciesInstalled() : false;
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

bool UStableDiffusionSubsystem::HasToken() const
{
	if (GeneratorBridge) {
		return !GeneratorBridge->GetToken().IsEmpty();
	}
	return false;
}

FString UStableDiffusionSubsystem::GetToken() const
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

void UStableDiffusionSubsystem::SetModelDirty()
{ 
	bIsModelDirty = true;
}

bool UStableDiffusionSubsystem::IsModelDirty() const
{
	return bIsModelDirty || GetModelStatus().ModelStatus != EModelStatus::Loaded;
}

void UStableDiffusionSubsystem::ConvertRawModel(UStableDiffusionModelAsset* InModelAsset, bool DeleteOriginal)
{
	if (GeneratorBridge) {
		this->GeneratorBridge->ConvertRawModel(InModelAsset, DeleteOriginal);
	}
}

void UStableDiffusionSubsystem::InitModel(
	const FStableDiffusionModelOptions& Model, 
	const FStableDiffusionPipelineOptions& Pipeline, 
	UStableDiffusionLORAAsset* LORAAsset, 
	UStableDiffusionTextualInversionAsset* TextualInversionAsset, 
	const TArray<FLayerProcessorContext>& Layers, 
	bool Async, 
	bool AllowNSFW, 
	EPaddingMode PaddingMode)
{
	if (GeneratorBridge) {
		// Unload any loaded models first
		//ReleaseModel();

		// Forward image updated event from bridge to subsystem
		this->GeneratorBridge->OnImageProgressEx.AddUniqueDynamic(this, &UStableDiffusionSubsystem::UpdateImageProgress);

		if (Async) {
			AsyncTask(ENamedThreads::AnyBackgroundHiPriTask, [this, Model, Pipeline, Layers, LORAAsset, TextualInversionAsset, AllowNSFW, PaddingMode]() mutable {
				auto Result = this->GeneratorBridge->InitModel(Model, Pipeline, LORAAsset, TextualInversionAsset, Layers, AllowNSFW, PaddingMode);
				if (Result.ModelStatus == EModelStatus::Loaded) {
					bIsModelDirty = false;
					ModelOptions = Model;
					PipelineOptions = Pipeline;
					//this->LORAAsset = LORAAsset;
				}

				AsyncTask(ENamedThreads::GameThread, [this, Result]() {
					this->OnModelInitializedEx.Broadcast(Result);
					});
				});
		}
		else {
			auto Result = this->GeneratorBridge->InitModel(Model, Pipeline, LORAAsset, TextualInversionAsset, Layers, AllowNSFW, PaddingMode);
			if (Result.ModelStatus == EModelStatus::Loaded) {
				ModelOptions = Model;
				PipelineOptions = Pipeline;
				bIsModelDirty = false;
			}

			AsyncTask(ENamedThreads::GameThread, [this, Result]() {
				this->OnModelInitializedEx.Broadcast(Result);
			});
		}
	}
}

void UStableDiffusionSubsystem::ReleaseModel()
{
	if (GeneratorBridge) {
		GeneratorBridge->ReleaseModel();
		this->GeneratorBridge->OnImageProgressEx.RemoveDynamic(this, &UStableDiffusionSubsystem::UpdateImageProgress);
	}
}

TArray<FString> UStableDiffusionSubsystem::GetCompatibleSchedulers() const
{
	TArray<FString> Result;
	if (GeneratorBridge) {
		Result = GeneratorBridge->AvailableSchedulers();
	}
	return Result;
}

FString UStableDiffusionSubsystem::GetCurrentScheduler() const
{
	if (GeneratorBridge) {
		return GeneratorBridge->GetScheduler();
	}
	return "";
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
			else if (ImageSourceType == EInputImageSource::Texture) {
				CaptureFromTextureSource(MoveTempIfPossible(Input));
			}

			// Restore screen messages and UI
			GAreScreenMessagesEnabled = bPrevGScreenMessagesEnabled;
			if (LevelEditorSubsystem)
				LevelEditorSubsystem->EditorSetGameView(bPrevViewportGameViewEnabled);
		});
}

void UStableDiffusionSubsystem::StopGeneratingImage()
{
	this->GeneratorBridge->StopImageGeneration();
	bIsGenerating = false;
}

void UStableDiffusionSubsystem::StartImageGeneration(FStableDiffusionInput Input)
{
	// Generate the image on a background thread
	//CurrentRenderTask = TGraphTask<FSDRenderTask>::CreateTask().ConstructAndDispatchWhenReady(ENamedThreads::AnyBackgroundHiPriTask, MoveTemp([this, Input]()
	AsyncTask(ENamedThreads::AnyBackgroundHiPriTask, [this, Input]()
		{
			// Generate image
			UTexture2D* OutTexture = UTexture2D::CreateTransient(Input.Options.OutSizeX, Input.Options.OutSizeY);
			UTexture2D* PreviewTexture = UTexture2D::CreateTransient(Input.Options.OutSizeX, Input.Options.OutSizeY);

			FStableDiffusionImageResult result = this->GeneratorBridge->GenerateImageFromStartImage(Input, OutTexture, PreviewTexture);

			bIsGenerating = false;

			// Create generated texture on game thread
			AsyncTask(ENamedThreads::GameThread, [this, result, OutTexture]
			{			
				UStableDiffusionBlueprintLibrary::UpdateTextureSync(OutTexture);
#if WITH_EDITOR
				OutTexture->PostEditChange();
#endif
				this->OnImageGenerationCompleteEx.Broadcast(result);
			});
	});
	//);
}

void UStableDiffusionSubsystem::UpsampleImage(const FStableDiffusionImageResult& input)
{
	if (!IsValid(GeneratorBridge) || !IsValid(input.OutTexture))
		return;

	bIsUpsampling = true;
	size_t upsampled_width = input.OutTexture->GetSizeX() * 4;
	size_t upsampled_height = input.OutTexture->GetSizeY() * 4;

	// Create output texature to hold our upsampled result
	UTexture2D* OutTexture = UTexture2D::CreateTransient(upsampled_width, upsampled_height);
	UStableDiffusionBlueprintLibrary::UpdateTextureSync(OutTexture);

	AsyncTask(ENamedThreads::AnyBackgroundHiPriTask, [this, input, upsampled_width, upsampled_height, OutTexture](){
		FTaskTagScope Scope(ETaskTag::EParallelRenderingThread);

		auto result = GeneratorBridge->UpsampleImage(input, OutTexture);
		bIsUpsampling = false;

		// Process result on game thread
		AsyncTask(ENamedThreads::GameThread, [this, result=MoveTemp(result), OutTexture]() {
			UStableDiffusionBlueprintLibrary::UpdateTextureSync(OutTexture);
#if WITH_EDITOR
			OutTexture->PostEditChange();
#endif
			OnImageUpsampleCompleteEx.Broadcast(result);
		});
	});
}

void UStableDiffusionSubsystem::UpdateImageProgress(int32 Step, int32 Timestep, float Progress, FIntPoint Size, UTexture2D* Texture)
{
	OnImageProgressUpdated.Broadcast(Step, Timestep, Progress, Size, Texture);
}

void UStableDiffusionSubsystem::SetLivePreviewEnabled(bool Enabled, float Delay, USceneCaptureComponent2D* Source)
{
	if (Enabled){
		if (Source) {
			if (!OnCaptureCameraUpdatedDlgHandle.IsValid()) {
				// Handle when capture component moves
				OnCaptureCameraUpdatedDlgHandle = Source->TransformUpdated.AddLambda([this, Delay, Source](USceneComponent* UpdatedComponent, EUpdateTransformFlags UpdateTransformFlags, ETeleportType Teleport) {
				
				FEditorCameraLivePreview CameraInfo;
				CameraInfo.Location = UpdatedComponent->GetComponentTransform().GetLocation();
				CameraInfo.Rotation = UpdatedComponent->GetComponentTransform().GetRotation().Rotator();
				CameraInfo.ViewportType = Source->ProjectionType == ECameraProjectionMode::Type::Perspective ? ELevelViewportType::LVT_Perspective : ELevelViewportType::LVT_OrthoFreelook;
				CameraInfo.ViewportIndex = 0;

				UE_LOG(LogTemp, Log, TEXT("Moving capture component to %s %s"), *CameraInfo.Location.ToString(), *CameraInfo.Rotation.ToString())

					// Only broadcast when the camera is not moving
					if (!(LastPreviewCameraInfo == CameraInfo)) {
						GEditor->GetTimerManager()->SetTimer(IdleCameraTimer, this, &UStableDiffusionSubsystem::LivePreviewUpdate, Delay, false);
					}

				LastPreviewCameraInfo = CameraInfo;
					});
			}
		}
		else {
			if (!OnCaptureCameraUpdatedDlgHandle.IsValid()) {
				OnCaptureCameraUpdatedDlgHandle = FEditorDelegates::OnEditorCameraMoved.AddLambda([this, Delay](const FVector& Location, const FRotator& Rotation, ELevelViewportType ViewportType, int32 ViewportIndex) {
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
		}
		
	}
	else if (!Enabled) {
		if (Source) {
			Source->TransformUpdated.Remove(OnCaptureCameraUpdatedDlgHandle);
		}
		else {
			FEditorDelegates::OnEditorCameraMoved.Remove(OnCaptureCameraUpdatedDlgHandle);
		}
		if (OnCaptureCameraUpdatedDlgHandle.IsValid()) {
			OnCaptureCameraUpdatedDlgHandle.Reset();
		}
	}
}

UTextureRenderTarget2D* UStableDiffusionSubsystem::SetLivePreviewForLayer(FIntPoint Size, ULayerProcessorBase* Layer, USceneCaptureComponent2D* CaptureSource)
{
	check(Layer);

	if (PreviewedLayer->IsValidLowLevel()) {
		DisableLivePreviewForLayer();
	}
	PreviewedLayer = Layer;

	USceneCaptureComponent2D* ActiveCaptureComponent = nullptr;
	
	// Assign or create the capture source
	if (CaptureSource->IsValidLowLevel()) {
		ActiveCaptureComponent = CaptureSource;
	}
	else {
		if (!LayerPreviewCapture.SceneCapture) {
			LayerPreviewCapture = CreateSceneCaptureFromEditorViewport();
			//DepthPreviewCapture.SceneCapture->SetIsTemporarilyHiddenInEditor(true);
			
			OnLayerPreviewUpdateHandle = FEditorDelegates::OnEditorCameraMoved.AddLambda([this](const FVector& Location, const FRotator& Rotation, ELevelViewportType ViewportType, int32 ViewportIndex) {
				UpdateSceneCaptureCamera(LayerPreviewCapture);

				// Update any previewed layers
				if (PreviewedLayer->IsValidLowLevel() && LayerPreviewCapture.SceneCapture->IsValidLowLevel())
					PreviewedLayer->CaptureLayer(LayerPreviewCapture.SceneCapture->GetCaptureComponent2D(), false);
			});
		}
		ActiveCaptureComponent = LayerPreviewCapture.SceneCapture->GetCaptureComponent2D();
	}

	// Start capturing the scene
	PreviewedLayer->BeginCaptureLayer(GEditor->GetWorld(), Size, ActiveCaptureComponent);
	return PreviewedLayer->CaptureLayer(CaptureSource, false);
}

void UStableDiffusionSubsystem::DisableLivePreviewForLayer()
{
	if (LayerPreviewCapture.SceneCapture && PreviewedLayer->IsValidLowLevel()) {
		if (PreviewedLayer)
			PreviewedLayer->EndCaptureLayer(GEditor->GetWorld(), LayerPreviewCapture.SceneCapture->GetCaptureComponent2D());

		AsyncTask(ENamedThreads::GameThread, [this]() {
			// Remove camera updater
			FEditorDelegates::OnEditorCameraMoved.Remove(OnLayerPreviewUpdateHandle);
			OnLayerPreviewUpdateHandle.Reset();

			LayerPreviewCapture.SceneCapture->Destroy();
			LayerPreviewCapture.SceneCapture = nullptr;
		});
	}

	PreviewedLayer = nullptr;
}

void UStableDiffusionSubsystem::ShowAspectOverlay()
{
	FActorSpawnParameters Params;
	Params.ObjectFlags |= RF_Transient;
	AspectOverlayActor = GEditor->GetEditorWorldContext().World()->SpawnActor<AActor>(AspectOverlayActorClass, FTransform::Identity, Params);
	if (AspectOverlayActor) {
		AspectOverlayActor->SetIsTemporarilyHiddenInEditor(true);
	}
}

void UStableDiffusionSubsystem::HideAspectOverlay()
{
	if (IsValid(AspectOverlayActor)) {
		AspectOverlayActor->Destroy();
	}
}

void UStableDiffusionSubsystem::UpdateAspectOverlay(float aspect)
{
	AspectOverlayValue = aspect;
	if (IsValid(AspectOverlayActor)) {
		/*AspectOverlayActor*/
	}
}

void UStableDiffusionSubsystem::CalculateOverlayBounds(float Aspect, FIntPoint& MinBounds, FIntPoint& MaxBounds)
{
	FIntRect Result;
	auto EditorViewport = UStableDiffusionSubsystem::GetCapturingViewport();
	auto Client = EditorViewport->GetClient();
	if (FEditorViewportClient* EditorClient = StaticCast<FEditorViewportClient*>(Client)) {
		FSceneViewFamilyContext ViewFamily(FSceneViewFamily::ConstructionValues(EditorViewport.Get(), EditorClient->GetScene(), EditorClient->EngineShowFlags));
		FSceneView* View = EditorClient->CalcSceneView(&ViewFamily);
		Result = EditorViewport->CalculateViewExtents(Aspect, View->UnconstrainedViewRect);
		MinBounds = Result.Min;
		MaxBounds = Result.Max;
	}
}

FViewportSceneCapture UStableDiffusionSubsystem::CreateSceneCaptureFromEditorViewport()
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
		SceneCapture.SceneCapture->GetCaptureComponent2D()->bCaptureEveryFrame = true;
		SceneCapture.SceneCapture->GetCaptureComponent2D()->bCaptureOnMovement = false;
		SceneCapture.SceneCapture->GetCaptureComponent2D()->bAlwaysPersistRenderingState = true;
		SceneCapture.SceneCapture->GetCaptureComponent2D()->CompositeMode = SCCM_Overwrite;
		SceneCapture.SceneCapture->GetCaptureComponent2D()->CaptureSource = ESceneCaptureSource::SCS_FinalToneCurveHDR;//ESceneCaptureSource::SCS_FinalToneCurveHDR;
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
}

void UStableDiffusionSubsystem::CaptureFromViewportSource(FStableDiffusionInput Input)
{
	auto ViewportSize = GetCapturingViewport()->GetSizeXY();

	FIntPoint MinBounds, MaxBounds;
	CalculateOverlayBounds(float(Input.Options.OutSizeX) / float(Input.Options.OutSizeY), MinBounds, MaxBounds);
	FIntRect FrameBounds(MinBounds.X, MinBounds.Y, MaxBounds.X, MaxBounds.Y);

	// Process each layer the model has requested
	if (Input.InputLayers.Num()) {
		Input.ProcessedLayers.Reset();
		Input.ProcessedLayers.Reserve(Input.InputLayers.Num());
		Input.View = UStableDiffusionBlueprintLibrary::GetEditorViewportViewInfo();

		// Create a scene capture
		auto SceneCapture = CreateSceneCaptureFromEditorViewport();

		for (auto& Layer : Input.InputLayers) {
			// Copy layer
			FLayerProcessorContext TargetLayer = Layer;
			TargetLayer.Processor->BeginCaptureLayer(SceneCapture.SceneCapture->GetWorld(), FrameBounds.Size(), SceneCapture.SceneCapture->GetCaptureComponent2D(), Layer.ProcessorOptions);
			auto ResultRT = TargetLayer.Processor->CaptureLayer(SceneCapture.SceneCapture->GetCaptureComponent2D(), true, Layer.ProcessorOptions);
			TargetLayer.Processor->EndCaptureLayer(SceneCapture.SceneCapture->GetWorld(), SceneCapture.SceneCapture->GetCaptureComponent2D());
			TargetLayer.LayerPixels = TargetLayer.Processor->ProcessLayer(ResultRT);
			Input.ProcessedLayers.Add(MoveTemp(TargetLayer));
		}

		// Cleanup
		SceneCapture.SceneCapture->Destroy();
	}

	// Take the screenshot of the active viewport
	TArray<FColor> Pixels;
	GetViewportScreenShot(UStableDiffusionSubsystem::GetCapturingViewport().Get(), Pixels, FrameBounds);

	// Find a final colour layer as a destination for our captured frame
	auto FinalColorProcessor = Input.ProcessedLayers.FindByPredicate([](const FLayerProcessorContext& Layer) { return Layer.Processor->IsA<UFinalColorLayerProcessor>(); });
	if (FinalColorProcessor) {
		FinalColorProcessor->LayerPixels = MoveTemp(Pixels);
	}

	// Set size from viewport
	Input.Options.InSizeX = FrameBounds.Size().X;
	Input.Options.InSizeY = FrameBounds.Size().Y;

	// Only start image generation when we have a frame
	StartImageGeneration(Input);
}

void UStableDiffusionSubsystem::CaptureFromSceneCaptureSource(FStableDiffusionInput Input)
{
	// Use chosen scene capture component or create a default one
	USceneCaptureComponent2D* CaptureComponent = nullptr;
	if (!Input.CaptureSource) {
		// Create a default SceneCapture2D that will capture our editor viewport
		CurrentSceneCapture = CreateSceneCaptureFromEditorViewport();
		CaptureComponent = CurrentSceneCapture.SceneCapture->GetCaptureComponent2D();
	}
	else {
		CaptureComponent = Input.CaptureSource;
	}


	// Hold onto camera view info
	FMinimalViewInfo CaptureSourceView;
	CaptureComponent->GetCameraView(0, CaptureSourceView);
	Input.View = CaptureSourceView;

	// Make sure input capture size is the same as our ouput texture size
	//auto ViewportSize = GetCapturingViewport()->GetSizeXY();
	FIntPoint CaptureSize(Input.Options.OutSizeX, Input.Options.OutSizeY);
	
	// Process each layer the model has requested
	Input.ProcessedLayers.Reset();
	Input.ProcessedLayers.Reserve(Input.InputLayers.Num());
	for (auto Layer : Input.InputLayers) {
		Layer.Processor->BeginCaptureLayer(CaptureComponent->GetWorld(), CaptureSize, CaptureComponent, Layer.ProcessorOptions);
		auto ResultRT = Layer.Processor->CaptureLayer(CaptureComponent, true, Layer.ProcessorOptions);
		Layer.Processor->EndCaptureLayer(CaptureComponent->GetWorld(), CaptureComponent);
		Layer.LayerPixels = Layer.Processor->ProcessLayer(ResultRT);
		Input.ProcessedLayers.Add(MoveTemp(Layer));
	}

	// Set size from scene capture
	Input.Options.InSizeX = CaptureSize.X;
	Input.Options.InSizeY = CaptureSize.Y;

	if (!Input.CaptureSource && this->CurrentSceneCapture.SceneCapture) {
		// Cleanup created scene capture once we've captured all our pixel data
		this->CurrentSceneCapture.SceneCapture->Destroy();
		this->CurrentSceneCapture.ViewportClient = nullptr;
	}
	else {
		
	}

	StartImageGeneration(Input);
}

void UStableDiffusionSubsystem::CaptureFromTextureSource(FStableDiffusionInput Input)
{
	if (!Input.OverrideTextureInput)
		return;

	// Make sure input capture size is the same as our ouput texture size
	FIntPoint CaptureSize(Input.Options.OutSizeX, Input.Options.OutSizeY);

	// Process each layer the model has requested
	Input.ProcessedLayers.Reset();
	Input.ProcessedLayers.Reserve(Input.InputLayers.Num());
	for (auto Layer : Input.InputLayers) {
		Layer.Processor->BeginCaptureLayer(GEditor->GetEditorWorldContext().World(), CaptureSize);
		auto ResultRT = Layer.Processor->CaptureLayer(nullptr);
		Layer.Processor->EndCaptureLayer(GEditor->GetEditorWorldContext().World());
		Layer.LayerPixels = Layer.Processor->ProcessLayer(ResultRT);
		Input.ProcessedLayers.Add(MoveTemp(Layer));
	}

	// Find a final colour layer as a destination for our texture
	auto FinalColorProcessor = Input.ProcessedLayers.FindByPredicate([](const FLayerProcessorContext& Layer) { return Layer.Processor->IsA<UFinalColorLayerProcessor>(); });
	if (FinalColorProcessor) {
		if (auto Tex = Input.OverrideTextureInput) {
			FinalColorProcessor->LayerPixels = UStableDiffusionBlueprintLibrary::ReadPixels(Input.OverrideTextureInput);
		}
	}

	StartImageGeneration(Input);
}


void UStableDiffusionSubsystem::OnLivePreviewCheckUpdate(USceneComponent* UpdatedComponent, EUpdateTransformFlags UpdateTransformFlags, ETeleportType Teleport) {

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

FStableDiffusionModelInitResult UStableDiffusionSubsystem::GetModelStatus() const
{
	if(GeneratorBridge)
		return GeneratorBridge->ModelStatus;
	return FStableDiffusionModelInitResult();
}

TArray<FColor> UStableDiffusionSubsystem::CopyFrameData(FIntRect Bounds, FIntPoint BufferSize, const FColor* ColorBuffer)
{
	// Copy frame data
	TArray<FColor> CopiedFrame;

	CopiedFrame.InsertUninitialized(0, Bounds.Size().X * Bounds.Size().Y);
	FColor* Dest = &CopiedFrame[0];
	const int32 MaxWidth = FMath::Min(Bounds.Size().X, BufferSize.X);

	ColorBuffer += Bounds.Min.Y * BufferSize.X;
	for (int32 Row = Bounds.Min.Y; Row < FMath::Min(Bounds.Max.Y, BufferSize.Y); ++Row)
	{
		FMemory::Memcpy(Dest, ColorBuffer + Bounds.Min.X, sizeof(FColor) * MaxWidth);
		ColorBuffer += BufferSize.X;
		Dest += MaxWidth;
	}

	return std::move(CopiedFrame);
}


#undef LOCTEXT_NAMESPACE
