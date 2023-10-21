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

bool UStableDiffusionSubsystem::RequiresToken() const
{
	if (GeneratorBridge) {
		return GeneratorBridge->GetRequiresToken();
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
	UStableDiffusionPipelineAsset* Pipeline, 
	UStableDiffusionLORAAsset* LORAAsset, 
	UStableDiffusionTextualInversionAsset* TextualInversionAsset, 
	const TArray<FLayerProcessorContext>& Layers, 
	bool Async, 
	bool AllowNSFW, 
	int32 SeamlessMode)
{
	if (GeneratorBridge) {
		// Unload any loaded models first
		//ReleaseModel();

		// Forward image updated event from bridge to subsystem
		this->GeneratorBridge->OnImageProgressEx.AddUniqueDynamic(this, &UStableDiffusionSubsystem::UpdateImageProgress);

		if (Async) {
			AsyncTask(ENamedThreads::AnyBackgroundHiPriTask, [this, Model, Pipeline, Layers, LORAAsset, TextualInversionAsset, AllowNSFW, SeamlessMode]() mutable {
				auto Result = this->GeneratorBridge->InitModel(Model, Pipeline, LORAAsset, TextualInversionAsset, Layers, AllowNSFW, SeamlessMode);
				if (Result.ModelStatus == EModelStatus::Loaded) {
					bIsModelDirty = false;
					ModelOptions = Model;
					PipelineAsset = Pipeline;
					//this->LORAAsset = LORAAsset;
				}

				AsyncTask(ENamedThreads::GameThread, [this, Result]() {
					this->OnModelInitializedEx.Broadcast(Result);
					});
				});
		}
		else {
			auto Result = this->GeneratorBridge->InitModel(Model, Pipeline, LORAAsset, TextualInversionAsset, Layers, AllowNSFW, SeamlessMode);
			if (Result.ModelStatus == EModelStatus::Loaded) {
				ModelOptions = Model;
				PipelineAsset = Pipeline;
				bIsModelDirty = false;
			}

			AsyncTask(ENamedThreads::GameThread, [this, Result]() {
				this->OnModelInitializedEx.Broadcast(Result);
			});
		}
	}
}

//void UStableDiffusionSubsystem::RunImagePipeline(TArray<UImagePipelineStageAsset*> Stages, FStableDiffusionInput Input, EInputImageSource ImageSourceType, bool Async, bool AllowNSFW, EPaddingMode PaddingMode)
//{
//	for (auto Stage : Stages) {
//		InitModel(Stage->Model->Options, Stage->Pipeline->Options, Stage->LORAAsset, Stage->TextualInversionAsset, Input.InputLayers, false, AllowNSFW, PaddingMode);
//		if (GetModelStatus().ModelStatus != EModelStatus::Loaded) {
//			UE_LOG(LogTemp, Error, TEXT("Failed to load model. Check the output log for more information"));
//			return;
//		}
//
//		GenerateImageSync(Input, ImageSourceType);
//	}
//}

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

void UStableDiffusionSubsystem::GenerateImage(FStableDiffusionInput Input, UImagePipelineStageAsset* PipelineStage, EInputImageSource ImageSourceType)
{
	if (!GeneratorBridge)
		return;

	AsyncTask(ENamedThreads::AnyBackgroundHiPriTask, [this, Input, PipelineStage, ImageSourceType]() mutable
	{
		GenerateImageSync(Input, PipelineStage, ImageSourceType);
	});
}

FStableDiffusionImageResult UStableDiffusionSubsystem::GenerateImageSync(FStableDiffusionInput Input, UImagePipelineStageAsset* PipelineStage, EInputImageSource ImageSourceType)
{
	FStableDiffusionImageResult Result;

	if (!GeneratorBridge)
		return Result;

	bIsGenerating = true;

	TSharedPtr<TPromise<bool>> GameThreadPromise = MakeShared<TPromise<bool>>();

	// Setup has to happen on the game thread
	AsyncTask(ENamedThreads::GameThread, [&]() 
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
			CaptureFromViewportSource(Input, PipelineStage);
		}
		else if (ImageSourceType == EInputImageSource::SceneCapture2D) {
			CaptureFromSceneCaptureSource(Input, PipelineStage);
		}
		else if (ImageSourceType == EInputImageSource::Texture) {
			CaptureFromTextureSource(Input, PipelineStage);
		}

		// Restore screen messages and UI
		GAreScreenMessagesEnabled = bPrevGScreenMessagesEnabled;
		if (LevelEditorSubsystem)
			LevelEditorSubsystem->EditorSetGameView(bPrevViewportGameViewEnabled);

		GameThreadPromise->SetValue(true);
	});

	// Block until game thread has finished setting up
	GameThreadPromise->GetFuture().Wait();

	Result = StartImageGenerationSync(Input, PipelineStage);
	return Result;
}

void UStableDiffusionSubsystem::StopGeneratingImage()
{
	bIsGenerating = false;
	bIsStopping = true;
	this->GeneratorBridge->StopImageGeneration();
}

bool UStableDiffusionSubsystem::IsStopping() const
{
	return bIsStopping;
}

void UStableDiffusionSubsystem::StartImageGeneration(FStableDiffusionInput Input, UImagePipelineStageAsset* PipelineStage)
{
	// Create empty texture 
	TArray<FColor> BlackPixels;
	BlackPixels.AddZeroed(Input.Options.OutSizeX * Input.Options.OutSizeY);
	FCreateTexture2DParameters CreateParams;
	CreateParams.bDeferCompression = true;
	CreateParams.CompressionSettings = TC_Default;
	CreateParams.bSRGB = false;
	CreateParams.bUseAlpha = false;
	CreateParams.MipGenSettings = TMGS_NoMipmaps;
	CreateParams.TextureGroup = TEXTUREGROUP_Pixels2D;
	
	UTexture2D* OutTexture = FImageUtils::CreateTexture2D(Input.Options.OutSizeX, Input.Options.OutSizeY, BlackPixels, PipelineStage, "OutTex", RF_Public | RF_Standalone, CreateParams);
	FAssetRegistryModule::AssetCreated(OutTexture);

	UTexture2D* PreviewTexture = UTexture2D::CreateTransient(Input.Options.OutSizeX, Input.Options.OutSizeY);

	// Generate the image on a background thread
	//CurrentRenderTask = TGraphTask<FSDRenderTask>::CreateTask().ConstructAndDispatchWhenReady(ENamedThreads::AnyBackgroundHiPriTask, MoveTemp([this, Input]()
	AsyncTask(ENamedThreads::AnyBackgroundHiPriTask, [this, Input, PipelineStage, OutTexture, PreviewTexture]()
	{
			FStableDiffusionImageResult result = this->GeneratorBridge->GenerateImageFromStartImage(Input, PipelineStage, OutTexture, PreviewTexture);

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

FStableDiffusionImageResult UStableDiffusionSubsystem::StartImageGenerationSync(FStableDiffusionInput Input, UImagePipelineStageAsset* PipelineStage)
{
	UTexture2D* OutTexture = nullptr;
	UTexture2D* PreviewTexture = nullptr;

	TArray<FColor> BlackPixels;
	BlackPixels.AddZeroed(Input.Options.OutSizeX * Input.Options.OutSizeY);

	TArray<FFloat16Color> BlackFloatPixels;
	BlackFloatPixels.AddZeroed(Input.Options.OutSizeX * Input.Options.OutSizeY);

	FCreateTexture2DParameters CreateParams;
	CreateParams.bDeferCompression = true;
	CreateParams.CompressionSettings = TC_Default;
	CreateParams.bSRGB = false;
	CreateParams.bUseAlpha = false;
	CreateParams.MipGenSettings = TMGS_NoMipmaps;
	CreateParams.TextureGroup = TEXTUREGROUP_Pixels2D;

	// Allocate output textures on the game thread
	TSharedPtr<TPromise<bool>> GameThreadPromise = MakeShared<TPromise<bool>>();
	AsyncTask(ENamedThreads::GameThread, [&]() {
		if (PipelineStage->Pipeline->Options.OutputTextureFormat == EPipelineOutputTextureFormat::FloatRGBA) {
			OutTexture = UStableDiffusionBlueprintLibrary::CreateHalfFloatTexture2D(Input.Options.OutSizeX, Input.Options.OutSizeY,  PipelineStage, "OutTex");
		}
		else {
			OutTexture = FImageUtils::CreateTexture2D(Input.Options.OutSizeX, Input.Options.OutSizeY, BlackPixels, PipelineStage, "OutTex", RF_Public | RF_Standalone, CreateParams);
		}
		PreviewTexture = UTexture2D::CreateTransient(Input.Options.OutSizeX, Input.Options.OutSizeY, (PipelineStage->Pipeline->Options.OutputTextureFormat == EPipelineOutputTextureFormat::FloatRGBA) ? EPixelFormat::PF_FloatRGBA : EPixelFormat::PF_B8G8R8A8);
		
		if (Input.SaveLayers) {
			for (auto& Layer : PipelineStage->Layers) {
				Layer.LayerTexture = FImageUtils::CreateTexture2D(Input.Options.OutSizeX, Input.Options.OutSizeY, BlackPixels, PipelineStage, "Layer", RF_Public | RF_Standalone, CreateParams);
			}
		}
		GameThreadPromise->SetValue(true);
	});
	GameThreadPromise->GetFuture().Wait();

	FStableDiffusionImageResult result = this->GeneratorBridge->GenerateImageFromStartImage(Input, PipelineStage, OutTexture, PreviewTexture);
	
	// Copy view info straight to the result
	result.View = Input.View;

	bIsGenerating = false;

	// Create generated texture on game thread
	AsyncTask(ENamedThreads::GameThread, [this, SaveLayers = Input.SaveLayers, PipelineStage, result, OutTexture]
	{
		UStableDiffusionBlueprintLibrary::UpdateTextureSync(OutTexture);
#if WITH_EDITOR
		FAssetRegistryModule::AssetCreated(OutTexture);
		OutTexture->PostEditChange();
#endif

		// Save layers
		if (SaveLayers) {
			for (auto& Layer : PipelineStage->Layers) {
				UStableDiffusionBlueprintLibrary::UpdateTextureSync(Layer.LayerTexture);
#if WITH_EDITOR
				FAssetRegistryModule::AssetCreated(Layer.LayerTexture);
				Layer.LayerTexture->PostEditChange();
#endif
			}
		}

		this->OnImageGenerationCompleteEx.Broadcast(result);
	});

	return result;
}

void UStableDiffusionSubsystem::ClearIsStopping()
{
	bIsStopping = false;
}

void UStableDiffusionSubsystem::UpsampleImage(const FStableDiffusionPipelineImageResult& input)
{
	if (!IsValid(GeneratorBridge) || !IsValid(input.ImageOutput.OutTexture))
		return;

	bIsUpsampling = true;
	size_t upsampled_width = input.ImageOutput.OutTexture->GetSizeX() * 4;
	size_t upsampled_height = input.ImageOutput.OutTexture->GetSizeY() * 4;

	// Need to detect if the image is linear from the game thread 
	bool IsLinear = UStableDiffusionBlueprintLibrary::IsTextureFloatFormat(input.ImageOutput.OutTexture);

	// Create output texture to hold our upsampled result
	UTexture2D* OutTexture = UTexture2D::CreateTransient(upsampled_width, upsampled_height);
	UStableDiffusionBlueprintLibrary::UpdateTextureSync(OutTexture);

	AsyncTask(ENamedThreads::AnyBackgroundHiPriTask, [this, input, IsLinear, OutTexture](){
		FTaskTagScope Scope(ETaskTag::EParallelRenderingThread);

		auto result = GeneratorBridge->UpsampleImage(input, OutTexture, IsLinear);
		bIsUpsampling = false;

		// Process result on game thread
		TSharedPtr<TPromise<bool>> GameThreadPromise = MakeShared<TPromise<bool>>();
		AsyncTask(ENamedThreads::GameThread, [this, result=MoveTemp(result), OutTexture, GameThreadPromise]() {
			UStableDiffusionBlueprintLibrary::UpdateTextureSync(OutTexture);
#if WITH_EDITOR
			OutTexture->PostEditChange();
#endif
			OnImageUpsampleCompleteEx.Broadcast(result);

			GameThreadPromise->SetValue(true);
		});
		GameThreadPromise->GetFuture().Wait();
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

void UStableDiffusionSubsystem::CaptureFromViewportSource(FStableDiffusionInput& Input, UImagePipelineStageAsset* PipelineStage)
{
	check(IsInGameThread());

	auto ViewportSize = GetCapturingViewport()->GetSizeXY();

	FIntPoint MinBounds, MaxBounds;
	CalculateOverlayBounds(float(Input.Options.OutSizeX) / float(Input.Options.OutSizeY), MinBounds, MaxBounds);
	FIntRect FrameBounds(MinBounds.X, MinBounds.Y, MaxBounds.X, MaxBounds.Y);
	
	// Save view info
	Input.View = UStableDiffusionBlueprintLibrary::GetEditorViewportViewInfo();
	
	// Process each layer the model has requested
	if (PipelineStage->Layers.Num()) {
		// Create a scene capture
		auto SceneCapture = CreateSceneCaptureFromEditorViewport();

		for (auto& Layer : PipelineStage->Layers) {
			// Copy layer
			FLayerProcessorContext TargetLayer = Layer;
			TargetLayer.Processor->BeginCaptureLayer(SceneCapture.SceneCapture->GetWorld(), FrameBounds.Size(), SceneCapture.SceneCapture->GetCaptureComponent2D(), Layer.ProcessorOptions);
			auto ResultRT = TargetLayer.Processor->CaptureLayer(SceneCapture.SceneCapture->GetCaptureComponent2D(), true, Layer.ProcessorOptions);
			TargetLayer.Processor->EndCaptureLayer(SceneCapture.SceneCapture->GetWorld(), SceneCapture.SceneCapture->GetCaptureComponent2D());
			TargetLayer.LayerPixels = TargetLayer.Processor->ProcessLayer(ResultRT);
		}

		// Cleanup
		SceneCapture.SceneCapture->Destroy();
	}

	// Take the screenshot of the active viewport
	TArray<FColor> Pixels;
	GetViewportScreenShot(UStableDiffusionSubsystem::GetCapturingViewport().Get(), Pixels, FrameBounds);

	// Find a final colour layer as a destination for our captured frame
	auto FinalColorProcessor = PipelineStage->Layers.FindByPredicate([](const FLayerProcessorContext& Layer) { return Layer.Processor->IsA<UFinalColorLayerProcessor>(); });
	if (FinalColorProcessor) {
		FinalColorProcessor->LayerPixels = MoveTemp(Pixels);
	}

	// Set size from viewport
	Input.Options.InSizeX = FrameBounds.Size().X;
	Input.Options.InSizeY = FrameBounds.Size().Y;
}

void UStableDiffusionSubsystem::CaptureFromSceneCaptureSource(FStableDiffusionInput& Input, UImagePipelineStageAsset* PipelineStage)
{
	check(IsInGameThread());

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
	for (auto Layer : PipelineStage->Layers) {
		Layer.Processor->BeginCaptureLayer(CaptureComponent->GetWorld(), CaptureSize, CaptureComponent, Layer.ProcessorOptions);
		auto ResultRT = Layer.Processor->CaptureLayer(CaptureComponent, true, Layer.ProcessorOptions);
		Layer.Processor->EndCaptureLayer(CaptureComponent->GetWorld(), CaptureComponent);
		Layer.LayerPixels = Layer.Processor->ProcessLayer(ResultRT);
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
}

void UStableDiffusionSubsystem::CaptureFromTextureSource(FStableDiffusionInput& Input, UImagePipelineStageAsset* PipelineStage)
{
	check(IsInGameThread());

	// Make sure input capture size is the same as our ouput texture size
	FIntPoint CaptureSize(Input.Options.OutSizeX, Input.Options.OutSizeY);

	// Process each layer the model has requested
	for (auto Layer : PipelineStage->Layers) {
		Layer.Processor->BeginCaptureLayer(GEditor->GetEditorWorldContext().World(), CaptureSize);
		auto ResultRT = Layer.Processor->CaptureLayer(nullptr);
		Layer.Processor->EndCaptureLayer(GEditor->GetEditorWorldContext().World());
		Layer.LayerPixels = Layer.Processor->ProcessLayer(ResultRT);
	}

	// Find a final colour layer as a destination for our texture
	auto FinalColorProcessor = PipelineStage->Layers.FindByPredicate([](const FLayerProcessorContext& Layer) { return Layer.Processor->IsA<UFinalColorLayerProcessor>(); });
	if (FinalColorProcessor) {
		if (auto Tex = Input.OverrideTextureInput) {
			FinalColorProcessor->LayerPixels = UStableDiffusionBlueprintLibrary::ReadPixels(Input.OverrideTextureInput);
		}
	}

	// Set size from texture
	if (UTexture2D* Tex2D = Cast<UTexture2D>(Input.OverrideTextureInput)) {
		Input.Options.InSizeX = Tex2D->GetSizeX();
		Input.Options.InSizeY = Tex2D->GetSizeY();
	}
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
