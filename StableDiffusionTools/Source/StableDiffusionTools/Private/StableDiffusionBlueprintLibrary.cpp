// Fill out your copyright notice in the Description page of Project Settings.

#include "StableDiffusionBlueprintLibrary.h"
#include "SLevelViewport.h"
#include "Dialogs/Dialogs.h"
#include "GeomTools.h"
#include "UObject/SavePackage.h"
#include "Components/SceneCaptureComponent2D.h"
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

FVector2d UStableDiffusionBlueprintLibrary::ProjectWorldToEditorViewportUV(const FVector& WorldPosition, bool& BehindCamera)
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



TArray<AActor*> UStableDiffusionBlueprintLibrary::GetActorsInViewFrustum(const UObject* WorldContextObject, const FMatrix& ViewProjectionMatrix)
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
	}
	return ActorsInFrustum;
}

UDynamicMesh* UStableDiffusionBlueprintLibrary::SetMeshUVsFromViewProjection(
	UDynamicMesh* TargetMesh,
	int UVSetIndex,
	FMatrix ViewProjMatrix,
	FGeometryScriptMeshSelection Selection,
	bool FrontFacingOnly,
	UGeometryScriptDebug* Debug)
{
	/*
	
	
	//if (TargetMesh == nullptr)
	//{
	//	//UE::Geometry::AppendError(Debug, EGeometryScriptErrorType::InvalidInputs, LOCTEXT("SetMeshUVsFromPlanarProjection_InvalidInput", "SetMeshUVsFromPlanarProjection: TargetMesh is Null"));
	//	return TargetMesh;
	//}
	
	bool bHasUVSet = false;
	/*ApplyMeshUVEditorOperation(TargetMesh, UVSetIndex, bHasUVSet, Debug,
		);

	bHasUVSet = false;
	TargetMesh->EditMesh([&](FDynamicMesh3& EditMesh)
	{
		if (EditMesh.HasAttributes() == false
			|| UVSetIndex >= EditMesh.Attributes()->NumUVLayers()
			|| EditMesh.Attributes()->GetUVLayer(UVSetIndex) == nullptr)
		{
			return;
		}

		bHasUVSet = true;
		FDynamicMeshUVOverlay* UVOverlay = EditMesh.Attributes()->GetUVLayer(UVSetIndex);
		//FDynamicMeshUVEditor Editor(&EditMesh, UVOverlay);
		
		//[&](FDynamicMesh3& EditMesh, FDynamicMeshUVOverlay* UVOverlay, FDynamicMeshUVEditor& UVEditor)
		{
			TArray<int32> TriangleROI;
			Selection.ProcessByTriangleID(EditMesh, [&](int32 TriangleID) { TriangleROI.Add(TriangleID); }, true);

			FFrame3d ProjectionFrame(PlaneTransform);
			FVector Scale = PlaneTransform.GetScale3D();
			FVector2d Dimensions(Scale.X, Scale.Y);

			//Editor.SetTriangleUVsFromPlanarProjection(TriangleROI, [&](const FVector3d& Pos) { return Pos; },
				ProjectionFrame, Dimensions);

			if (ensure(UVOverlay) == false) return;
			if (!Triangles.Num()) return;

			ResetUVs(Triangles);

			double ScaleX = (FMathd::Abs(Dimensions.X) > FMathf::ZeroTolerance) ? (1.0 / Dimensions.X) : 1.0;
			double ScaleY = (FMathd::Abs(Dimensions.Y) > FMathf::ZeroTolerance) ? (1.0 / Dimensions.Y) : 1.0;

			TMap<int32, int32> BaseToOverlayVIDMap;
			TArray<int32> NewUVIndices;

			for (int32 TID : Triangles)
			{
				FIndex3i BaseTri = EditMesh.GetTriangle(TID);
				FIndex3i ElemTri;
				for (int32 j = 0; j < 3; ++j)
				{
					const int32* FoundElementID = BaseToOverlayVIDMap.Find(BaseTri[j]);
					if (FoundElementID == nullptr)
					{
						FVector3d Pos = EditMesh.GetVertex(BaseTri[j]);
						FVector3d TransformPos = PointTransform(Pos);
						FVector2d UV = ProjectionFrame.ToPlaneUV(TransformPos, 2);
						UV.X *= ScaleX;
						UV.Y *= ScaleY;
						ElemTri[j] = UVOverlay->AppendElement(FVector2f(UV));
						NewUVIndices.Add(ElemTri[j]);
						BaseToOverlayVIDMap.Add(BaseTri[j], ElemTri[j]);
					}
					else
					{
						ElemTri[j] = *FoundElementID;
					}
				}
				UVOverlay->SetTriangle(TID, ElemTri);
			}

			if (Result != nullptr)
			{
				Result->NewUVElements = MoveTemp(NewUVIndices);
			}
		}

	}, EDynamicMeshChangeType::GeneralEdit, EDynamicMeshAttributeChangeFlags::Unknown, false);


	if (bHasUVSet == false)
	{
		//UE::Geometry::AppendError(Debug, EGeometryScriptErrorType::InvalidInputs, LOCTEXT("SetMeshUVsFromPlanarProjection_InvalidUVSet", "SetMeshUVsFromPlanarProjection: UVSetIndex does not exist on TargetMesh"));
	}

	*/
	return TargetMesh;
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


void UStableDiffusionBlueprintLibrary::CopyTextureDataUsingUVs(UTexture2D* SourceTexture, UTexture2D* TargetTexture, const TMap<int, FGeometryScriptUVTriangle>& SourceUVs, const TMap<int, FGeometryScriptUVTriangle>& TargetUVs)
{
	if (!SourceTexture || !TargetTexture || !SourceUVs.Num() || !TargetUVs.Num())
	{
		UE_LOG(LogTemp, Error, TEXT("CopyTexturePixels: Invalid input parameters"));
		return;
	}

	// Get source and target texture sizes
	const int32 SourceWidth = SourceTexture->GetSizeX();
	const int32 SourceHeight = SourceTexture->GetSizeY();
	const int32 TargetWidth = TargetTexture->GetSizeX();
	const int32 TargetHeight = TargetTexture->GetSizeY();

	// Lock the source and target texture for reading/writing
	//FColor* TargetTextureData = static_cast<FColor*>(TargetTexture->Source.LockMip(0));
	TArray<FColor> TargetColors;
	TargetColors.InsertUninitialized(0, TargetWidth * TargetHeight);

	// Iterate through each triangle and copy pixels from source to target
	for(auto TriPair : TargetUVs)
	//for (int32 i = 0; i < TargetUVs.Num(); ++i)
	{
		if(!SourceUVs.Contains(TriPair.Key))
			continue;

		FVector2D SourceVertex1 = SourceUVs[TriPair.Key].UV0;
		FVector2D SourceVertex2 = SourceUVs[TriPair.Key].UV1;
		FVector2D SourceVertex3 = SourceUVs[TriPair.Key].UV2; 
		FVector2D TargetVertex1 = TriPair.Value.UV0;
		FVector2D TargetVertex2 = TriPair.Value.UV1;
		FVector2D TargetVertex3 = TriPair.Value.UV2;

		// Convert source and target UVs to pixel coordinates
		int32 SourceX1 = FMath::FloorToInt(SourceVertex1.X * SourceWidth);
		int32 SourceY1 = FMath::FloorToInt(SourceVertex1.Y * SourceHeight);
		int32 SourceX2 = FMath::FloorToInt(SourceVertex2.X * SourceWidth);
		int32 SourceY2 = FMath::FloorToInt(SourceVertex2.Y * SourceHeight);
		int32 SourceX3 = FMath::FloorToInt(SourceVertex3.X * SourceWidth);
		int32 SourceY3 = FMath::FloorToInt(SourceVertex3.Y * SourceHeight);
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
						FVector2D SourceUV(
							SourceVertex1.X * BarycentricCoords.X + SourceVertex2.X * BarycentricCoords.Y + SourceVertex3.X * BarycentricCoords.Z,
							SourceVertex1.Y * BarycentricCoords.X + SourceVertex2.Y * BarycentricCoords.Y + SourceVertex3.Y * BarycentricCoords.Z
						);

						// Convert source UV to pixel coordinates
						int32 SourceX = FMath::FloorToInt(SourceUV.X * SourceWidth);
						int32 SourceY = FMath::FloorToInt(SourceUV.Y * SourceHeight);

						// Copy pixel color from source to target
						if (SourceX >= 0 && SourceX < SourceWidth && SourceY >= 0 && SourceY < SourceHeight)
						{
							int32 SourceIndex = (SourceY * SourceWidth + SourceX); // 4 channels (RGBA)

							// Copy RGBA values from source to target
							TargetColors[TargetIndex] = GetUVPixelFromTexture(SourceTexture, SourceUV);// SourceTextureData[SourceIndex];
						}
					}
				}
			}
		}
	}

	// Copy colors to target texture
	ColorBufferToTexture(TargetColors, FIntPoint(TargetWidth, TargetHeight), TargetTexture);
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

UTexture2D*UStableDiffusionBlueprintLibrary::CreateTextureAsset(const FString& AssetPath, const FString& Name, FIntPoint Size, FColor Fill)
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
	NewTexture->Source.Init(Size.X, Size.Y, 1, 1, ETextureSourceFormat::TSF_BGRA8, FillData);//ETextureSourceFormat::TSF_RGBA8);
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
