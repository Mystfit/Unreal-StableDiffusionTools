// Fill out your copyright notice in the Description page of Project Settings.

#include "StableDiffusionBlueprintLibrary.h"
#include "SLevelViewport.h"
#include "Dialogs/Dialogs.h"
#include "GeomTools.h"
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
#include "ImageCoreUtils.h"

using namespace UE::Geometry;

#define LOCTEXT_NAMESPACE "StableDiffusionBlueprintLibrary"

UStableDiffusionSubsystem* UStableDiffusionBlueprintLibrary::GetStableDiffusionSubsystem() {
	UStableDiffusionSubsystem* subsystem = nullptr;
	subsystem = GEditor->GetEditorSubsystem<UStableDiffusionSubsystem>();
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

	auto EditorViewport = UStableDiffusionSubsystem::GetCapturingViewport();
	auto Client = EditorViewport->GetClient();
	if (FEditorViewportClient* EditorClient = StaticCast<FEditorViewportClient*>(Client)) {
		FSceneViewFamilyContext ViewFamily(FSceneViewFamily::ConstructionValues(EditorViewport.Get(), EditorClient->GetScene(), EditorClient->EngineShowFlags));
		FSceneView* View = EditorClient->CalcSceneView(&ViewFamily);
		

		//// Orig projection method - seems to break with verts behind the camera
		//View->WorldToPixel(WorldPosition, Result);
		
		auto ProjPlane = View->Project(WorldPosition);
		Result.X = ProjPlane.X * 0.5f + 0.5f;
		Result.Y = ProjPlane.Y * -0.5f + 0.5f;
		BehindCamera = ProjPlane.W < 0.0f;
	}
	return Result;
}

FMatrix UStableDiffusionBlueprintLibrary::GetEditorViewportViewProjectionMatrix()
{
	auto EditorViewport = UStableDiffusionSubsystem::GetCapturingViewport();
	auto Client = EditorViewport->GetClient();
	if (FEditorViewportClient* EditorClient = StaticCast<FEditorViewportClient*>(Client)) {
		FSceneViewFamilyContext ViewFamily(FSceneViewFamily::ConstructionValues(EditorViewport.Get(), EditorClient->GetScene(), EditorClient->EngineShowFlags));
		FSceneView* View = EditorClient->CalcSceneView(&ViewFamily);
		return View->ViewMatrices.GetViewProjectionMatrix();
	}

	return FMatrix::Identity;
}

FTransform UStableDiffusionBlueprintLibrary::GetEditorViewportCameraTransform()
{
	auto EditorViewport = UStableDiffusionSubsystem::GetCapturingViewport();
	auto Client = EditorViewport->GetClient();
	if (FEditorViewportClient* EditorClient = StaticCast<FEditorViewportClient*>(Client)) {
		FSceneViewFamilyContext ViewFamily(FSceneViewFamily::ConstructionValues(EditorViewport.Get(), EditorClient->GetScene(), EditorClient->EngineShowFlags));
		FSceneView* View = EditorClient->CalcSceneView(&ViewFamily);
		return FTransform(View->ViewRotation, View->ViewLocation);
	}	
	
	return FTransform();
}

FMatrix UStableDiffusionBlueprintLibrary::GetEditorViewportViewMatrix()
{
	auto EditorViewport = UStableDiffusionSubsystem::GetCapturingViewport();
	auto Client = EditorViewport->GetClient();
	if (FEditorViewportClient* EditorClient = StaticCast<FEditorViewportClient*>(Client)) {
		FSceneViewFamilyContext ViewFamily(FSceneViewFamily::ConstructionValues(EditorViewport.Get(), EditorClient->GetScene(), EditorClient->EngineShowFlags));
		FSceneView* View = EditorClient->CalcSceneView(&ViewFamily);
		return View->ViewMatrices.GetViewMatrix();
	}

	return FMatrix::Identity;
}

FIntPoint UStableDiffusionBlueprintLibrary::GetEditorViewportSize()
{
	auto EditorViewport = UStableDiffusionSubsystem::GetCapturingViewport();
	auto Client = EditorViewport->GetClient();
	if (FEditorViewportClient* EditorClient = StaticCast<FEditorViewportClient*>(Client)) {
		FSceneViewFamilyContext ViewFamily(FSceneViewFamily::ConstructionValues(EditorViewport.Get(), EditorClient->GetScene(), EditorClient->EngineShowFlags));
		FSceneView* View = EditorClient->CalcSceneView(&ViewFamily);
		return FIntPoint(View->UnconstrainedViewRect.Width(), View->UnconstrainedViewRect.Height());
	}
	return FIntPoint(0, 0);
}

FVector UStableDiffusionBlueprintLibrary::GetEditorViewportDirection()
{
	auto EditorViewport = UStableDiffusionSubsystem::GetCapturingViewport();
	auto Client = EditorViewport->GetClient();
	if (FEditorViewportClient* EditorClient = StaticCast<FEditorViewportClient*>(Client)) {
		FSceneViewFamilyContext ViewFamily(FSceneViewFamily::ConstructionValues(EditorViewport.Get(), EditorClient->GetScene(), EditorClient->EngineShowFlags));
		FSceneView* View = EditorClient->CalcSceneView(&ViewFamily);
		return View->GetViewDirection();
	}

	return FVector::ForwardVector;
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

UTexture2D* UStableDiffusionBlueprintLibrary::ColorBufferToTexture(const TArray<FColor>& FrameColors, const FIntPoint& FrameSize, UTexture2D* OutTex)
{
	if (!FrameColors.Num())
		return nullptr;
	return ColorBufferToTexture((uint8*)FrameColors.GetData(), FrameSize, OutTex);
}

UTexture2D* UStableDiffusionBlueprintLibrary::ColorBufferToTexture(const uint8* FrameData, const FIntPoint& FrameSize, UTexture2D* OutTex)
{
	if (!FrameData) 
		return nullptr;

	ETextureSourceFormat TexFormat = ETextureSourceFormat::TSF_BGRA8;
	if (OutTex) {
		TexFormat = OutTex->Source.GetFormat();
	} else {
		TObjectPtr<UTexture2D> NewTex = UTexture2D::CreateTransient(FrameSize.X, FrameSize.Y, EPixelFormat::PF_B8G8R8A8);
		OutTex = NewTex;
	}

	OutTex->Source.Init(FrameSize.X, FrameSize.Y, 1, 1, TexFormat);//ETextureSourceFormat::TSF_RGBA8);
	OutTex->MipGenSettings = TMGS_NoMipmaps;
	OutTex->SRGB = true;
	OutTex->DeferCompression = true;

	ERawImageFormat::Type ImgFormat = FImageCoreUtils::ConvertToRawImageFormat(OutTex->Source.GetFormat());
	int NumChannels = GPixelFormats[FImageCoreUtils::GetPixelFormatForRawImageFormat(ImgFormat)].NumComponents;

	uint8* TextureData = OutTex->Source.LockMip(0);
	FMemory::Memcpy(TextureData, FrameData, sizeof(uint8) * FrameSize.X * FrameSize.Y * NumChannels);
	OutTex->Source.UnlockMip(0);
	OutTex->UpdateResource();

#if WITH_EDITOR
	OutTex->PostEditChange();
#endif
	return OutTex;
}

void UStableDiffusionBlueprintLibrary::CopyTextureDataUsingUVs(UTexture2D* CoverageTexture, UTexture2D* SourceTexture, UTexture2D* TargetTexture, const FIntPoint& ScreenSize, const FMatrix& ViewProjectionMatrix, UDynamicMesh* SourceMesh, const TArray<int> TriangleIDs)
{
	if (!SourceTexture || !TargetTexture || ViewProjectionMatrix == FMatrix::Identity  || !SourceMesh || !TriangleIDs.Num() || !CoverageTexture)
	{
		UE_LOG(LogTemp, Error, TEXT("CopyTexturePixels: Invalid input parameters"));
		return;
	}
	
	ERawImageFormat::Type ImgFormat = FImageCoreUtils::ConvertToRawImageFormat(CoverageTexture->Source.GetFormat());
	EPixelFormat CoveragePixelFormat = FImageCoreUtils::GetPixelFormatForRawImageFormat(ImgFormat);
	if (GPixelFormats[CoveragePixelFormat].NumComponents > 1){
		UE_LOG(LogTemp, Error, TEXT("Coverage texture has too many channels. Expected a single channel"));
		return;
	}

	// Hack. Get the editor viewport
	FSceneView* View = nullptr;
	auto EditorViewport = UStableDiffusionSubsystem::GetCapturingViewport();
	auto Client = EditorViewport->GetClient();
	if (FEditorViewportClient* EditorClient = StaticCast<FEditorViewportClient*>(Client)) {
		FSceneViewFamilyContext ViewFamily(FSceneViewFamily::ConstructionValues(EditorViewport.Get(), EditorClient->GetScene(), EditorClient->EngineShowFlags));
		View = EditorClient->CalcSceneView(&ViewFamily);
	}

	// Get source and target texture sizes
	const int32 SourceWidth = SourceTexture->GetSizeX();
	const int32 SourceHeight = SourceTexture->GetSizeY();
	const int32 TargetWidth = TargetTexture->GetSizeX();
	const int32 TargetHeight = TargetTexture->GetSizeY();

	// Lock the source and target texture for reading/writing
	TArray<FColor> TargetColors;
	TargetColors.InsertUninitialized(0, TargetWidth * TargetHeight);

	// Get the coverage texture pixels
	TArray<uint8_t> CoveragePixels;
	CoveragePixels.InsertUninitialized(0, TargetWidth * TargetHeight);

	// Iterate through each triangle and copy pixels from source to target
	for(auto TriID : TriangleIDs)
	{
		FVector2D TargetVertex1;
		FVector2D TargetVertex2;
		FVector2D TargetVertex3;
		bool ValidUVs;
		UGeometryScriptLibrary_MeshQueryFunctions::GetTriangleUVs(SourceMesh, 0, TriID, TargetVertex1, TargetVertex2, TargetVertex3, ValidUVs);

		// Get WS tri vertex positions
		FVector SourceVertex1;
		FVector SourceVertex2;
		FVector SourceVertex3;
		bool ValidPositions;
		UGeometryScriptLibrary_MeshQueryFunctions::GetTrianglePositions(SourceMesh, TriID, ValidPositions, SourceVertex1, SourceVertex2, SourceVertex3);

		// Convert source and target UVs to pixel coordinates
		int32 TargetX1 = FMath::FloorToInt(TargetVertex1.X * TargetWidth);
		int32 TargetY1 = FMath::FloorToInt(TargetVertex1.Y * TargetHeight);
		int32 TargetX2 = FMath::FloorToInt(TargetVertex2.X * TargetWidth);
		int32 TargetY2 = FMath::FloorToInt(TargetVertex2.Y * TargetHeight);
		int32 TargetX3 = FMath::FloorToInt(TargetVertex3.X * TargetWidth);
		int32 TargetY3 = FMath::FloorToInt(TargetVertex3.Y * TargetHeight);

		// Iterate through target pixels within the triangle bounds
		for (int32 X = FMath::Min3(TargetX1, TargetX2, TargetX3); X <= FMath::Max3(TargetX1, TargetX2, TargetX3); ++X)
		{
			for (int32 Y = FMath::Min3(TargetY1, TargetY2, TargetY3); Y <= FMath::Max3(TargetY1, TargetY2, TargetY3); ++Y)
			{
				if (X >= 0 && X < TargetWidth && Y >= 0 && Y < TargetHeight) 
				{
					int32 TargetIndex = (Y * TargetWidth + X);

					// Get target UV coordinate
					FVector2f TargetUV = FVector2f(X, Y) / FVector2f(TargetWidth, TargetHeight);

					// Check if the pixel is inside the triangle
					TArray<FVector2D> TargetVerts{ TargetVertex1, TargetVertex2, TargetVertex3 };
					if (FGeomTools2D::IsPointInPolygon(FVector2D(TargetUV), TargetVerts))
					{
						// Interpolate source UVs using barycentric coordinates
						FVector BarycentricCoords = FMath::ComputeBaryCentric2D(FVector(TargetUV.X, TargetUV.Y, 0.0f), FVector(TargetVertex1.X, TargetVertex1.Y, 0.0f), FVector(TargetVertex2.X, TargetVertex2.Y, 0.0f), FVector(TargetVertex3.X, TargetVertex3.Y, 0.0f));
						FVector SourceWSPos(
							SourceVertex1.X * BarycentricCoords.X + SourceVertex2.X * BarycentricCoords.Y + SourceVertex3.X * BarycentricCoords.Z,
							SourceVertex1.Y * BarycentricCoords.X + SourceVertex2.Y * BarycentricCoords.Y + SourceVertex3.Y * BarycentricCoords.Z,
							SourceVertex1.Z * BarycentricCoords.X + SourceVertex2.Z * BarycentricCoords.Y + SourceVertex3.Z * BarycentricCoords.Z
						);

						FVector2D OutPixel(-1.0f, -1.0f);
						View->ScreenToPixel(View->WorldToScreen(SourceWSPos), OutPixel);

						if (OutPixel.X >= 0 && OutPixel.X < View->UnconstrainedViewRect.Width() && OutPixel.Y >= 0 && OutPixel.Y < View->UnconstrainedViewRect.Height()) {
							FVector2D SourceUV(
								OutPixel.X / View->UnconstrainedViewRect.Width(),
								OutPixel.Y / View->UnconstrainedViewRect.Height()
							);

							int32 SourceX = SourceUV.X * SourceWidth;
							int32 SourceY = SourceUV.X * SourceHeight;

							/*UE_LOG(LogTemp, Log, TEXT("-----------------------------------"));
							UE_LOG(LogTemp, Log, TEXT("Interpolated vertex X:%d, Y:%d Z:%d"), SourceWSPos.X, SourceWSPos.Y, SourceWSPos.Z);
							UE_LOG(LogTemp, Log, TEXT("Screenspace coord X:%d, Y:%d"), OutPixel.X, OutPixel.Y);
							UE_LOG(LogTemp, Log, TEXT("UV coord U:%d, V:%d"), SourceUV.X, SourceUV.Y);*/

							// Copy pixel color from source to target
							if (SourceX >= 0 && SourceX < SourceWidth && SourceY >= 0 && SourceY < SourceHeight)
							{
								int32 SourceIndex = (SourceY * SourceWidth + SourceX); // 4 channels (RGBA)

								// Copy RGBA values from source to target
								TargetColors[TargetIndex] = GetUVPixelFromTexture(SourceTexture, FVector2D(SourceUV));// SourceTextureData[SourceIndex];
								CoveragePixels[TargetIndex] = 255;
							}
						}
					}
				}
			}
		}
	}

	// Update coverage texture
	// Unlock the render target texture and update it with the modified pixel values

	// Copy colors to target textures
	ColorBufferToTexture(TargetColors, FIntPoint(TargetWidth, TargetHeight), TargetTexture);
	ColorBufferToTexture(CoveragePixels.GetData(), FIntPoint(TargetWidth, TargetHeight), CoverageTexture);
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

UTexture2D*UStableDiffusionBlueprintLibrary::CreateTextureAsset(const FString& AssetPath, const FString& Name, FIntPoint Size, ETextureSourceFormat Format, FColor Fill)
{
	if (AssetPath.IsEmpty() || Name.IsEmpty())
		return nullptr;

	// Create package
	FString FullPackagePath = FPaths::Combine(AssetPath, Name);
	UPackage* Package = CreatePackage(*FullPackagePath);
	Package->FullyLoad();
	Package->AddToRoot();

	// Duplicate texture
	TArray<FColor> FillPixels;
	FillPixels.Init(Fill, Size.X * Size.Y);
	uint8_t* FillData = (uint8_t*)(FillPixels.GetData());

	UTexture2D* NewTexture = NewObject<UTexture2D>(Package, *Name, RF_Public | RF_Standalone);
	NewTexture->Source.Init(Size.X, Size.Y, 1, 1, Format, FillData);//ETextureSourceFormat::TSF_RGBA8);
	NewTexture->MipGenSettings = TMGS_NoMipmaps;
	NewTexture->SRGB = true;
	NewTexture->DeferCompression = true;

	NewTexture->UpdateResource();
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

FColor UStableDiffusionBlueprintLibrary::LerpColor(const FColor& ColorA, const FColor& ColorB, float Alpha)
{
	return FColor(
		FMath::Lerp(ColorA.R, ColorB.R, Alpha),
		FMath::Lerp(ColorA.G, ColorB.G, Alpha),
		FMath::Lerp(ColorA.B, ColorB.B, Alpha),
		FMath::Lerp(ColorA.A, ColorB.A, Alpha)
	);
}
