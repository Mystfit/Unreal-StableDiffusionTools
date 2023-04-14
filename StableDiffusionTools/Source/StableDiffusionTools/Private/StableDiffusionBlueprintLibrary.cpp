// Fill out your copyright notice in the Description page of Project Settings.

#include "StableDiffusionBlueprintLibrary.h"
#include "SLevelViewport.h"
#include "Dialogs/Dialogs.h"
#include "Components/SceneCaptureComponent2D.h"
#include "ConvexVolume.h"
#include "EditorViewportClient.h"
#include "EngineUtils.h"
#include "LevelEditor.h"
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

FVector2d UStableDiffusionBlueprintLibrary::ProjectWorldToEditorViewportUV(const FVector& WorldPosition)
{
	FVector2d Result;

	auto EditorViewport = UStableDiffusionSubsystem::GetCapturingViewport();
	auto Client = EditorViewport->GetClient();
	if (FEditorViewportClient* EditorClient = StaticCast<FEditorViewportClient*>(Client)) {
		FSceneViewFamilyContext ViewFamily(FSceneViewFamily::ConstructionValues(EditorViewport.Get(), EditorClient->GetScene(), EditorClient->EngineShowFlags));
		FSceneView* View = EditorClient->CalcSceneView(&ViewFamily);

		auto ScreenPosition = View->WorldToScreen(WorldPosition);
		float Y = (GProjectionSignY > 0.0f) ? ScreenPosition.Y : 1.0f - ScreenPosition.Y;
		Result = FVector2D(
			View->UnscaledViewRect.Min.X + (0.5f + ScreenPosition.X * 0.5f),
			View->UnscaledViewRect.Min.Y + (0.5f - Y * 0.5f)
		);
		
		//View->WorldToPixel(WorldPosition, ScreenPosition);
		
		// Convert to viewport UV space
		//ScreenPosition /= EditorViewport->GetSizeXY();
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
