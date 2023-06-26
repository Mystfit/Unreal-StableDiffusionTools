// Fill out your copyright notice in the Description page of Project Settings.

#include "StableDiffusionBlueprintLibrary.h"
#include "SLevelViewport.h"
#include "Dialogs/Dialogs.h"
#include "GeomTools.h"
#include "TextureCompiler.h"
#include "UObject/SavePackage.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Camera/CameraTypes.h"
#include "ConvexVolume.h"
#include "EditorViewportClient.h"
#include "EngineUtils.h"
#include "LevelEditor.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Engine/Texture2D.h"
#include "Rendering/Texture2DResource.h"
#include "Kismet/GameplayStatics.h"
#include "GeometryScript/GeometryScriptSelectionTypes.h"
#include "Parameterization/DynamicMeshUVEditor.h"
#include "GeometryScript/MeshQueryFunctions.h"
#include "ProjectionBakeSession.h"
#include "ImageCoreUtils.h"
#include "IAssetTools.h"
#include "MaterialEditingLibrary.h"
#include "Factories/MaterialInstanceConstantFactoryNew.h"
#include "AssetToolsModule.h"

using namespace UE::Geometry;

#define LOCTEXT_NAMESPACE "StableDiffusionBlueprintLibrary"

UStableDiffusionSubsystem* UStableDiffusionBlueprintLibrary::GetStableDiffusionSubsystem() {
	UStableDiffusionSubsystem* subsystem = nullptr;
	subsystem = GEditor->GetEditorSubsystem<UStableDiffusionSubsystem>();
	return subsystem;
}

ULayersSubsystem* UStableDiffusionBlueprintLibrary::GetLayersSubsystem() {
	ULayersSubsystem* subsystem = nullptr;
	subsystem = GEditor->GetEditorSubsystem<ULayersSubsystem>();
	return subsystem;
}

UStableDiffusionToolsSettings* UStableDiffusionBlueprintLibrary::GetPluginOptions()
{
	auto result = GetMutableDefault<UStableDiffusionToolsSettings>();
	return result;
}

UDependencySettings* UStableDiffusionBlueprintLibrary::GetDependencyOptions()
{
	auto result = GetMutableDefault<UDependencySettings>();
	return result;
}

void UStableDiffusionBlueprintLibrary::RestartEditor()
{
	// Present the user with a warning that changing projects has to restart the editor
	FSuppressableWarningDialog::FSetupInfo Info(
		LOCTEXT("RestartEditorMsg", "The editor needs to restart to complete an operation."),
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


FVector2D UStableDiffusionBlueprintLibrary::ProjectSceneCaptureWorldToUV(const FVector& WorldPosition, USceneCaptureComponent2D* SceneCapture, bool& BehindCamera)
{
	FVector2d Result;
	UE_LOG(LogTemp, Error, TEXT("ProjectSceneCaptureWorldToUV() not implemented"));
	return Result;
}

FVector2d UStableDiffusionBlueprintLibrary::ProjectViewportWorldToUV(const FVector& WorldPosition, bool& BehindCamera)
{
	FVector2d Result;

	if (FSceneView* View = UStableDiffusionBlueprintLibrary::CalculateEditorView(UStableDiffusionSubsystem::GetCapturingViewport().Get())) {
		auto ProjPlane = View->Project(WorldPosition);
		Result.X = ProjPlane.X * 0.5f + 0.5f;
		Result.Y = ProjPlane.Y * -0.5f + 0.5f;
		BehindCamera = ProjPlane.W < 0.0f;
	}
	return Result;
}

FMatrix UStableDiffusionBlueprintLibrary::GetEditorViewportViewProjectionMatrix()
{
	if (FSceneView* View = UStableDiffusionBlueprintLibrary::CalculateEditorView(UStableDiffusionSubsystem::GetCapturingViewport().Get())) {
		return View->ViewMatrices.GetViewProjectionMatrix();
	}

	return FMatrix::Identity;
}

FTransform UStableDiffusionBlueprintLibrary::GetEditorViewportCameraTransform()
{
	if (FSceneView* View = UStableDiffusionBlueprintLibrary::CalculateEditorView(UStableDiffusionSubsystem::GetCapturingViewport().Get())) {
		return FTransform(View->ViewRotation, View->ViewLocation);
	}	
	
	return FTransform();
}

FMatrix UStableDiffusionBlueprintLibrary::GetEditorViewportViewMatrix()
{
	if (FSceneView* View = UStableDiffusionBlueprintLibrary::CalculateEditorView(UStableDiffusionSubsystem::GetCapturingViewport().Get())) {
		return View->ViewMatrices.GetViewMatrix();
	}

	return FMatrix::Identity;
}

FMinimalViewInfo UStableDiffusionBlueprintLibrary::GetEditorViewportViewInfo()
{
	FMinimalViewInfo ViewInfo;
	if (FSceneView* View = UStableDiffusionBlueprintLibrary::CalculateEditorView(UStableDiffusionSubsystem::GetCapturingViewport().Get())) {
		ViewInfo.AspectRatio = (float)View->UnconstrainedViewRect.Width() / (float)View->UnconstrainedViewRect.Height();
		ViewInfo.FOV = View->FOV;
		ViewInfo.Location = View->ViewLocation;
		ViewInfo.PostProcessSettings = View->FinalPostProcessSettings;
		ViewInfo.ProjectionMode = (View->IsPerspectiveProjection()) ? ECameraProjectionMode::Type::Perspective : ECameraProjectionMode::Type::Orthographic;
		ViewInfo.Rotation = View->ViewRotation;
	}
	return ViewInfo;
}


FIntPoint UStableDiffusionBlueprintLibrary::GetEditorViewportSize()
{
	if (FSceneView* View = UStableDiffusionBlueprintLibrary::CalculateEditorView(UStableDiffusionSubsystem::GetCapturingViewport().Get())) {
		return FIntPoint(View->UnconstrainedViewRect.Width(), View->UnconstrainedViewRect.Height());
	}
	return FIntPoint(0, 0);
}

FVector UStableDiffusionBlueprintLibrary::GetEditorViewportDirection()
{
	if (FSceneView* View = UStableDiffusionBlueprintLibrary::CalculateEditorView(UStableDiffusionSubsystem::GetCapturingViewport().Get())) {
		return View->GetViewDirection();
	}
	return FVector::ForwardVector;
}

void UStableDiffusionBlueprintLibrary::SetEditorViewportRealtimeOverride(bool Realtime)
{
	if (auto EditorClient = UStableDiffusionBlueprintLibrary::GetEditorClient()) {
		EditorClient->AddRealtimeOverride(Realtime, FText::FromString("Stable Diffusion Tools"));
	}
}

void UStableDiffusionBlueprintLibrary::ClearEditorViewportRealtimeOverride()
{
	if (auto EditorClient = UStableDiffusionBlueprintLibrary::GetEditorClient()) {
		EditorClient->RemoveRealtimeOverride();
	}
}

bool UStableDiffusionBlueprintLibrary::GetEditorViewportRealtime()
{
	if (auto EditorClient = UStableDiffusionBlueprintLibrary::GetEditorClient()) {
		return EditorClient->IsRealtime();
	}
	return true;
}

TArray<AActor*> UStableDiffusionBlueprintLibrary::GetActorsInViewFrustum(const UObject* WorldContextObject, const FMatrix& ViewProjectionMatrix, const FVector& CameraLocation)
{
	TArray<AActor*> ActorsInFrustum;
	if (WorldContextObject)
	{
		// Get the camera's view frustum planes

		FConvexVolume ViewFrustum;
		GetViewFrustumBounds(ViewFrustum, ViewProjectionMatrix, false);

		// Iterate through all actors in the world
		for (TActorIterator<AActor> ActorItr(WorldContextObject->GetWorld()); ActorItr; ++ActorItr)
		{
			AActor* Actor = *ActorItr;
			if (Actor && !Actor->IsPendingKill())
			{
				FVector Center;
				FVector Extents;
				Actor->GetComponentsBoundingBox().GetCenterAndExtents(Center, Extents);
				// Check if the actor's bounding box is fully inside the view frustum
				if (ViewFrustum.IntersectBox(Center, Extents))
				{
					ActorsInFrustum.Add(Actor);
				}
			}
		}

		ActorsInFrustum.Sort([&CameraLocation](const AActor& A, const AActor& B){
			float DistanceA = FVector::Distance(A.GetActorLocation(), CameraLocation);
			float DistanceB = FVector::Distance(B.GetActorLocation(), CameraLocation);
			return DistanceA < DistanceB;
		});
	}
	return ActorsInFrustum;
}

UTexture2D* UStableDiffusionBlueprintLibrary::ColorBufferToTexture(const TArray<FColor>& FrameColors, const FIntPoint& FrameSize, UTexture2D* OutTex, bool DeferUpdate)
{
	if (!FrameColors.Num())
		return nullptr;
	return ColorBufferToTexture((uint8*)FrameColors.GetData(), FrameSize, OutTex, DeferUpdate);
}

TArray<FColor> UStableDiffusionBlueprintLibrary::ReadPixels(UTexture* Texture)
{
	TArray<FColor> Pixels;

	if (!IsValid(Texture))
		return Pixels;

	if (auto Tex2D = Cast<UTexture2D>(Texture)) {
		auto Mip = Tex2D->GetPlatformData()->Mips[0];
		FColor* RawPixels = static_cast<FColor*>(Mip.BulkData.Lock(LOCK_READ_ONLY));
		Pixels = TArray<FColor>(RawPixels, Mip.BulkData.GetBulkDataSize() / 4);
		Mip.BulkData.Unlock();
	}
	else if (auto RenderTarget2D = Cast<UTextureRenderTarget2D>(Texture)) {
		FTextureRenderTargetResource* RT_Resource = nullptr;
		if (IsInGameThread()) {
			RT_Resource = RenderTarget2D->GameThread_GetRenderTargetResource();
		}
		else if(IsInRenderingThread()){
			RT_Resource = RenderTarget2D->GetRenderTargetResource();
		}
		
		RT_Resource->ReadPixels(Pixels, FReadSurfaceDataFlags(RCM_MinMax));
	}

	return MoveTemp(Pixels);
}

void UStableDiffusionBlueprintLibrary::UpdateTextureSync(UTexture* Texture)
{
	if(!IsValid(Texture))
		return;

	Texture->UpdateResource();

	int NumChecks = 25;

#if WITH_EDITOR
	FTextureCompilingManager::Get().FinishCompilation({ Texture });
#endif

	Texture->SetForceMipLevelsToBeResident(30.0f);
	Texture->WaitForStreaming();
}

UTexture2D* UStableDiffusionBlueprintLibrary::ColorBufferToTexture(const uint8* FrameData, const FIntPoint& FrameSize, UTexture2D* OutTex, bool DeferUpdate)
{
	if (!FrameData) 
		return nullptr;

	ETextureSourceFormat TexFormat = ETextureSourceFormat::TSF_BGRA8;
	if (OutTex) {
		TexFormat = (OutTex->Source.GetFormat() != ETextureSourceFormat::TSF_Invalid) ? OutTex->Source.GetFormat() : TSF_BGRA8;
	} else {
		TObjectPtr<UTexture2D> NewTex = UTexture2D::CreateTransient(FrameSize.X, FrameSize.Y, EPixelFormat::PF_B8G8R8A8);
		OutTex = NewTex;
		//UE_LOG(LogTemp, Log, TEXT("Creating transient texture to hold color buffer. Input Width: %d, Height: %d. Texture Width: %d, Height: %d"), FrameSize.X, FrameSize.Y, NewTex->GetSizeX(), NewTex->GetSizeY());
	}

	OutTex->Source.Init(FrameSize.X, FrameSize.Y, 1, 1, TexFormat);//ETextureSourceFormat::TSF_RGBA8);
	OutTex->MipGenSettings = TMGS_NoMipmaps;
	OutTex->SRGB = true;
	OutTex->DeferCompression = true;
	OutTex->CompressionSettings = TextureCompressionSettings::TC_VectorDisplacementmap;

	ERawImageFormat::Type ImgFormat = FImageCoreUtils::ConvertToRawImageFormat(OutTex->Source.GetFormat());
	int NumChannels = GPixelFormats[FImageCoreUtils::GetPixelFormatForRawImageFormat(ImgFormat)].NumComponents;

	uint8* TextureData = OutTex->Source.LockMip(0);
	FMemory::Memcpy(TextureData, FrameData, sizeof(uint8) * FrameSize.X * FrameSize.Y * NumChannels);
	OutTex->Source.UnlockMip(0);

	if (!DeferUpdate) {
		OutTex->UpdateResource();
#if WITH_EDITOR
		OutTex->PostEditChange();
#endif
	}
	return OutTex;
}

FString UStableDiffusionBlueprintLibrary::LayerTypeToString(ELayerImageType LayerType)
{
	if(ULayerProcessorBase::ReverseLayerImageTypeLookup.Contains(LayerType))
		return ULayerProcessorBase::ReverseLayerImageTypeLookup[LayerType];
	return "";
}

ELayerImageType UStableDiffusionBlueprintLibrary::StringToLayerType(FString LayerName)
{
	if (ULayerProcessorBase::LayerImageTypeLookup.Contains(LayerName))
		return ULayerProcessorBase::LayerImageTypeLookup[LayerName];
	return ELayerImageType::unknown;
}

FEditorViewportClient* UStableDiffusionBlueprintLibrary::GetEditorClient()
{
	if (auto EditorViewport = UStableDiffusionSubsystem::GetCapturingViewport()) {
		auto Client = EditorViewport->GetClient();
		if (FEditorViewportClient* EditorClient = StaticCast<FEditorViewportClient*>(Client)) {
			return EditorClient;
		}
	}
	
	return nullptr;
}

FSceneView* UStableDiffusionBlueprintLibrary::CalculateEditorView(FSceneViewport* EditorViewport)
{
	if (EditorViewport) {
		auto Client = EditorViewport->GetClient();
		if (FEditorViewportClient* EditorClient = StaticCast<FEditorViewportClient*>(Client)) {
			FSceneViewFamilyContext ViewFamily(FSceneViewFamily::ConstructionValues(EditorViewport, EditorClient->GetScene(), EditorClient->EngineShowFlags));
			return EditorClient->CalcSceneView(&ViewFamily);
		}
	}
	return nullptr;
}

UStableDiffusionStyleModelAsset* UStableDiffusionBlueprintLibrary::CreateModelAsset(const FString& PackagePath, const FString& Name)
{
	if (Name.IsEmpty() || PackagePath.IsEmpty())
		return false;

	// Create package
	FString FullPackagePath = FPaths::Combine(PackagePath, Name);
	UPackage* Package = CreatePackage(*FullPackagePath);
	Package->FullyLoad();

	// Create data asset
	UStableDiffusionStyleModelAsset* NewModelAsset = NewObject<UStableDiffusionStyleModelAsset>(Package, *Name, RF_Public | RF_Standalone);

	// Update package
	Package->MarkPackageDirty();
	FAssetRegistryModule::AssetCreated(NewModelAsset);

	return NewModelAsset;
}

UStableDiffusionLORAAsset* UStableDiffusionBlueprintLibrary::CreateLORAAsset(const FString& PackagePath, const FString& Name)
{
	if (Name.IsEmpty() || PackagePath.IsEmpty())
		return false;

	// Create package
	FString FullPackagePath = FPaths::Combine(PackagePath, Name);
	UPackage* Package = CreatePackage(*FullPackagePath);
	Package->FullyLoad();

	// Create data asset
	UStableDiffusionLORAAsset* NewLORAAsset = NewObject<UStableDiffusionLORAAsset>(Package, *Name, RF_Public | RF_Standalone);

	// Update package
	Package->MarkPackageDirty();
	FAssetRegistryModule::AssetCreated(NewLORAAsset);

	return NewLORAAsset;
}

UStableDiffusionImageResultAsset* UStableDiffusionBlueprintLibrary::CreateImageResultAsset(const FString& PackagePath, const FString& Name, UTexture2D* Texture, FIntPoint Size, const FStableDiffusionImageResult& ImageResult, FMinimalViewInfo View, bool Upsampled)
{
	if (Name.IsEmpty() || PackagePath.IsEmpty() || !Texture)
		return false;

	// Create package
	FString FullPackagePath = FPaths::Combine(PackagePath, Name);
	UPackage* Package = CreatePackage(*FullPackagePath);
	Package->FullyLoad();

	// Duplicate texture
	auto SrcMipData = Texture->Source.LockMip(0);// GetPlatformMips()[0].BulkData;
	FString TexName = "T_" + Name;
	UTexture2D* NewTexture = NewObject<UTexture2D>(Package, *TexName, RF_Public | RF_Standalone);
	NewTexture = UStableDiffusionBlueprintLibrary::ColorBufferToTexture(SrcMipData, Size, NewTexture);
	Texture->Source.UnlockMip(0);

	// Create data asset
	FString AssetName = "DA_" + Name;
	UStableDiffusionImageResultAsset* NewModelAsset = NewObject<UStableDiffusionImageResultAsset>(Package, *AssetName, RF_Public | RF_Standalone);
	NewModelAsset->ImageOutput = ImageResult;
	NewModelAsset->ImageInputs = ImageResult.Input.Options;
	NewModelAsset->ImageOutput.Upsampled = Upsampled;
	NewModelAsset->ImageOutput.OutTexture = NewTexture;
	NewModelAsset->ImageOutput.View = View;

	// Update package
	Package->MarkPackageDirty();
	FAssetRegistryModule::AssetCreated(NewTexture);

	//// Save texture pacakge
	//FString PackageFileName = FPackageName::LongPackageNameToFilename(FullPackagePath, FPackageName::GetAssetPackageExtension());
	//FSavePackageArgs PackageArgs;
	//PackageArgs.TopLevelFlags = EObjectFlags::RF_Public | EObjectFlags::RF_Standalone;
	//PackageArgs.bForceByteSwapping = true;
	//bool bSaved = UPackage::SavePackage(Package, NewTexture, *PackageFileName, PackageArgs);

	return NewModelAsset;
}

void UStableDiffusionBlueprintLibrary::CopyTextureDataUsingUVs(UTexture2D* SourceTexture, UTexture2D* TargetTexture, const FIntPoint& ScreenSize, const FMatrix& ViewProjectionMatrix, UDynamicMesh* SourceMesh, const TArray<int> TriangleIDs, bool ClearCoverageMask)
{
	if (!SourceTexture || !TargetTexture || ViewProjectionMatrix == FMatrix::Identity  || !SourceMesh || !TriangleIDs.Num())
	{
		UE_LOG(LogTemp, Error, TEXT("CopyTexturePixels: Invalid input parameters"));
		return;
	}

	// Hack. Get the editor viewport
	FSceneView* View = UStableDiffusionBlueprintLibrary::CalculateEditorView(UStableDiffusionSubsystem::GetCapturingViewport().Get());

	// Get source and target texture sizes
	const int32 SourceWidth = SourceTexture->GetSizeX();
	const int32 SourceHeight = SourceTexture->GetSizeY();
	const int32 TargetWidth = TargetTexture->GetSizeX();
	const int32 TargetHeight = TargetTexture->GetSizeY();
	
	// Lock the target texture for reading/writing
	// 
	//FTexture2DMipMap* Mip = &TargetTexture->PlatformData->Mips[0];
	//FColor* TargetTextureRawData = static_cast<FColor*>(Mip->BulkData.Lock(LOCK_READ_ONLY));
	//check(TargetTextureRawData);

	// Fill interim pixel array
	TArray<FColor> TargetPixelColors = ReadPixels(TargetTexture);
	//TargetPixelColors.Init(FColor(0, 0, 0, 0), TargetWidth * TargetHeight);
	//for (size_t idx = 0; idx < TargetPixelColors.Num(); ++idx) {
	//	TargetPixelColors[idx] = TargetTextureRawData[idx];
	//}
	//Mip->BulkData.Unlock();

	// Iterate through each triangle and copy pixels from source to target
	for(auto TriID : TriangleIDs)
	{
		// The target triangle vertices in UV space we will write our pixel to
		FVector2D TargetVertex1UV;
		FVector2D TargetVertex2UV;
		FVector2D TargetVertex3UV;
		bool ValidUVs;
		UGeometryScriptLibrary_MeshQueryFunctions::GetTriangleUVs(SourceMesh, 0, TriID, TargetVertex1UV, TargetVertex2UV, TargetVertex3UV, ValidUVs);

		// The equivalent worldspace positions of the same vertices for the current mesh.
		// Currently assuming the triangles are already in world space (when duplicating the source mesh to a dynamic mesh)
		// TODO: Passin in component transform to calculate vertices in worldspace
		FVector SourceVertex1;
		FVector SourceVertex2;
		FVector SourceVertex3;
		bool ValidPositions;
		UGeometryScriptLibrary_MeshQueryFunctions::GetTrianglePositions(SourceMesh, TriID, ValidPositions, SourceVertex1, SourceVertex2, SourceVertex3);

		// Convert target UVs to pixel within the target texture
		int32 TargetX1 = FMath::FloorToInt(TargetVertex1UV.X * TargetWidth);
		int32 TargetY1 = FMath::FloorToInt(TargetVertex1UV.Y * TargetHeight);
		int32 TargetX2 = FMath::FloorToInt(TargetVertex2UV.X * TargetWidth);
		int32 TargetY2 = FMath::FloorToInt(TargetVertex2UV.Y * TargetHeight);
		int32 TargetX3 = FMath::FloorToInt(TargetVertex3UV.X * TargetWidth);
		int32 TargetY3 = FMath::FloorToInt(TargetVertex3UV.Y * TargetHeight);

		// Iterate through pixels in the target texture within a bounding box surrouding the current triangle
		for (int32 X = FMath::Min3(TargetX1, TargetX2, TargetX3); X <= FMath::Max3(TargetX1, TargetX2, TargetX3); ++X)
		{
			for (int32 Y = FMath::Min3(TargetY1, TargetY2, TargetY3); Y <= FMath::Max3(TargetY1, TargetY2, TargetY3); ++Y)
			{
				// Make sure we don't sample from pixels outside of the texture just in case the triangle goes off the edge
				if (X >= 0 && X < TargetWidth && Y >= 0 && Y < TargetHeight) 
				{
					// Get the UV coordinate of the current pixel in the target texture
					FVector2f TargetUV = FVector2f(X, Y) / FVector2f(TargetWidth, TargetHeight);

					// Check if the current pixel is inside the triangle
					TArray<FVector2D> TargetVerts{ TargetVertex1UV, TargetVertex2UV, TargetVertex3UV };
					if (FGeomTools2D::IsPointInPolygon(FVector2D(TargetUV), TargetVerts))
					{
						// To convert the current texture space pixel coordinate into screenspace coordinates, we calculate the barycentric weights of the current
						// pixel within the current triangle and then apply these weights to the worldspace coordinates of the source vertexes in order to figure out the WS coordinate
						// of the current pixel
						FVector BarycentricCoords = FMath::ComputeBaryCentric2D(FVector(TargetUV.X, TargetUV.Y, 0.0f), FVector(TargetVertex1UV.X, TargetVertex1UV.Y, 0.0f), FVector(TargetVertex2UV.X, TargetVertex2UV.Y, 0.0f), FVector(TargetVertex3UV.X, TargetVertex3UV.Y, 0.0f));
						FVector SourceWSPos(
							SourceVertex1.X * BarycentricCoords.X + SourceVertex2.X * BarycentricCoords.Y + SourceVertex3.X * BarycentricCoords.Z,
							SourceVertex1.Y * BarycentricCoords.X + SourceVertex2.Y * BarycentricCoords.Y + SourceVertex3.Y * BarycentricCoords.Z,
							SourceVertex1.Z * BarycentricCoords.X + SourceVertex2.Z * BarycentricCoords.Y + SourceVertex3.Z * BarycentricCoords.Z
						);

						// Convert the WS coords of the current pixel into screenspace so that we can sample from the projected camera texture
						FVector2D OutPixel(-1.0f, -1.0f);
						View->ScreenToPixel(View->WorldToScreen(SourceWSPos), OutPixel);

						// Make sure the pixel lies within the source texture
						if (OutPixel.X >= 0 && OutPixel.X < View->UnconstrainedViewRect.Width() && OutPixel.Y >= 0 && OutPixel.Y < View->UnconstrainedViewRect.Height()) {
							
							// Transform the pixel coordinate from screenspace to source texture splace
							FVector2D SourceUV(
								OutPixel.X / View->UnconstrainedViewRect.Width(),
								OutPixel.Y / View->UnconstrainedViewRect.Height()
							);
							int32 SourceX = SourceUV.X * SourceWidth;
							int32 SourceY = SourceUV.Y * SourceHeight;

							// Copy pixel color from source to target
							if (SourceX >= 0 && SourceX < SourceWidth && SourceY >= 0 && SourceY < SourceHeight)
							{
								// Copy RGBA values from source to target
								int32 SourceIndex = (SourceY * SourceWidth + SourceX);
								int32 TargetIndex = (Y * TargetWidth + X);// *4;

								// Only write to pixels that haven't already been written to in a previous capture pass
								if (TargetPixelColors[TargetIndex].A < 255 || ClearCoverageMask) {
									TargetPixelColors[TargetIndex] = GetUVPixelFromTexture(SourceTexture, FVector2D(SourceUV));
								}
							}
						}
					}
				}
			}
		}
	}

	ColorBufferToTexture(TargetPixelColors, FIntPoint(TargetWidth, TargetHeight), TargetTexture);
}

FColor UStableDiffusionBlueprintLibrary::GetUVPixelFromTexture(UTexture2D* Texture, FVector2D UV)
{
	if (!Texture)
	{
		// Handle null texture case
		return FColor::Black;
	}

	// Get texture size
	int32 TextureWidth = Texture->GetSizeX();
	int32 TextureHeight = Texture->GetSizeY();

	// Calculate pixel coordinates from UV
	float PixelX = UV.X * TextureWidth;
	float PixelY = UV.Y * TextureHeight;

	// Get the four surrounding pixels
	int32 PixelLeft = FMath::Clamp(FMath::FloorToInt(PixelX), 0, TextureWidth - 1);
	int32 PixelRight = FMath::Clamp(FMath::CeilToInt(PixelX), 0, TextureWidth - 1);
	int32 PixelUp = FMath::Clamp(FMath::FloorToInt(PixelY), 0, TextureHeight - 1);
	int32 PixelDown = FMath::Clamp(FMath::CeilToInt(PixelY), 0, TextureHeight - 1);

	// Get raw texture data
	FTexture2DMipMap* Mip = &Texture->PlatformData->Mips[0];
	FByteBulkData* RawImageData = &Mip->BulkData;
	FColor* RawData = static_cast<FColor*>(RawImageData->Lock(LOCK_READ_ONLY));

	// Read pixel colors from raw data
	FColor PixelColorUpperLeft = RawData[PixelUp * TextureWidth + PixelLeft];
	FColor PixelColorUpperRight = RawData[PixelUp * TextureWidth + PixelRight];
	FColor PixelColorLowerLeft = RawData[PixelDown * TextureWidth + PixelLeft];
	FColor PixelColorLowerRight = RawData[PixelDown * TextureWidth + PixelRight];

	// Interpolate colors based on UV distance
	float LerpX = PixelX - PixelLeft;
	float LerpY = PixelY - PixelUp;

	auto TopRowColor = LerpColor(PixelColorUpperLeft, PixelColorUpperRight, LerpX);
	auto BottomRowColor = LerpColor(PixelColorLowerLeft, PixelColorLowerRight, LerpX);
	FColor InterpolatedColor = LerpColor(TopRowColor, BottomRowColor, LerpY);

	// Lock texture
	RawImageData->Unlock();

	return InterpolatedColor;
}

UTexture2D* UStableDiffusionBlueprintLibrary::CreateTransientTexture(int32 InSizeX, int32 InSizeY, EPixelFormat InFormat, const FName InName) {
	auto Tex =  UTexture2D::CreateTransient(InSizeX, InSizeY, InFormat, InName);
	UpdateTextureSync(Tex);
	return Tex;
}

UTexture2D* UStableDiffusionBlueprintLibrary::CreateTextureAsset(const FString& AssetPath, const FString& Name, FIntPoint Size, ETextureSourceFormat Format, FColor Fill)
{
	if (AssetPath.IsEmpty() || Name.IsEmpty() || Size.GetMin() < 1)
		return nullptr;

	// Create package
	FString FullPackagePath = FPaths::Combine(AssetPath, Name);
	UPackage* Package = CreatePackage(*FullPackagePath);
	check(Package);
	Package->FullyLoad();
	Package->AddToRoot();

	// Init fill data
	TArray<FColor> FillPixels;
	FillPixels.Init(Fill, Size.X * Size.Y);
	uint8_t* FillData = (uint8_t*)(FillPixels.GetData());

	ERawImageFormat::Type ImgFormat = FImageCoreUtils::ConvertToRawImageFormat(Format);
	UTexture2D* NewTexture = UTexture2D::CreateTransient(Size.X, Size.Y, FImageCoreUtils::GetPixelFormatForRawImageFormat(ImgFormat), FName(Name));
	//UTexture2D* NewTexture = NewObject<UTexture2D>(Package, *Name, RF_Public | RF_Standalone);
	
	// Make sure transient texture sticks around.
	NewTexture->ClearFlags(RF_Transient);
	NewTexture->SetFlags(EObjectFlags::RF_Public | EObjectFlags::RF_Standalone);
	NewTexture->Rename(nullptr, Package);

	NewTexture->SRGB = true;
#if WITH_EDITOR
	NewTexture->MipGenSettings = TMGS_NoMipmaps;
	NewTexture->DeferCompression = true;
	NewTexture->CompressionNone = true;
#endif
	// Copy into source
	NewTexture->Source.Init(Size.X, Size.Y, 1, 1, Format, FillData);//ETextureSourceFormat::TSF_RGBA8);

	// Copy into target mip as well
	auto MipData = NewTexture->GetPlatformData()->Mips[0].BulkData.Lock(LOCK_READ_WRITE);
	FMemory::Memcpy(MipData, FillData, FillPixels.Num());
	NewTexture->GetPlatformData()->Mips[0].BulkData.Unlock();

	NewTexture->UpdateResource();
	NewTexture->MarkPackageDirty();
	NewTexture->Modify();
#if WITH_EDITOR
	NewTexture->PostEditChange();
#endif
	check(NewTexture->Source.IsValid());

	// Update package
	Package->MarkPackageDirty();
	FAssetRegistryModule::AssetCreated(NewTexture);

	// Save texture pacakge
	FString PackageFileName = FPackageName::LongPackageNameToFilename(FullPackagePath, FPackageName::GetAssetPackageExtension());
	FSavePackageArgs PackageArgs;
	PackageArgs.TopLevelFlags = EObjectFlags::RF_Public | EObjectFlags::RF_Standalone;
	PackageArgs.bForceByteSwapping = true;
	bool bSaved = UPackage::SavePackage(Package, NewTexture, *PackageFileName, PackageArgs);

	return NewTexture;
}

UProjectionBakeSessionAsset* UStableDiffusionBlueprintLibrary::CreateProjectionBakeSessionAsset(const FProjectionBakeSession& Session, const FString& AssetPath, const FString& Name)
{
	if (AssetPath.IsEmpty() || Name.IsEmpty())
		return nullptr;

	// Create package
	FString FullPackagePath = FPaths::Combine(AssetPath, Name);
	UPackage* Package = CreatePackage(*FullPackagePath);
	Package->FullyLoad();

	UProjectionBakeSessionAsset* NewSessionAsset = NewObject<UProjectionBakeSessionAsset>(Package, *Name, RF_Public | RF_Standalone);
	check(NewSessionAsset);
	NewSessionAsset->Session = Session;
#if WITH_EDITOR
	NewSessionAsset->PostEditChange();
#endif

	// Update package
	Package->MarkPackageDirty();
	FAssetRegistryModule::AssetCreated(NewSessionAsset);

	// Save session package
	/*FString PackageFileName = FPackageName::LongPackageNameToFilename(FullPackagePath, FPackageName::GetAssetPackageExtension());
	FSavePackageArgs PackageArgs;
	PackageArgs.TopLevelFlags = EObjectFlags::RF_Public | EObjectFlags::RF_Standalone;
	PackageArgs.bForceByteSwapping = true;
	bool bSaved = UPackage::SavePackage(Package, NewSessionAsset, *PackageFileName, PackageArgs);*/

	return NewSessionAsset;
}

UMaterialInstanceConstant* UStableDiffusionBlueprintLibrary::CreateMaterialInstanceAsset(UMaterial* ParentMaterial, const FString& Path, const FString& Name)
{
	UMaterialInterface* MasterMaterial = ParentMaterial;
	if (MasterMaterial == nullptr)
		return nullptr;

	IAssetTools& AssetTools = FModuleManager::GetModuleChecked<FAssetToolsModule>("AssetTools").Get();
	UMaterialInstanceConstantFactoryNew* Factory = NewObject<UMaterialInstanceConstantFactoryNew>();
	UMaterialInstanceConstant* MaterialInstance = CastChecked<UMaterialInstanceConstant>(AssetTools.CreateAsset(Name, Path, UMaterialInstanceConstant::StaticClass(), Factory));
	UMaterialEditingLibrary::SetMaterialInstanceParent(MaterialInstance, MasterMaterial);
	if (MaterialInstance)
	{
		MaterialInstance->SetFlags(RF_Standalone);
		MaterialInstance->MarkPackageDirty();
		MaterialInstance->PostEditChange();
	}
	return MaterialInstance;
}

FColor UStableDiffusionBlueprintLibrary::LerpColor(const FColor& ColorA, const FColor& ColorB, float Alpha)
{
	return FColor(
		FMath::Lerp(ColorA.R, ColorB.R, Alpha),
		FMath::Lerp(ColorA.G, ColorB.G, Alpha),
		FMath::Lerp(ColorA.B, ColorB.B, Alpha),
		FMath::Lerp(ColorA.A, ColorB.A, Alpha)
	);
}
