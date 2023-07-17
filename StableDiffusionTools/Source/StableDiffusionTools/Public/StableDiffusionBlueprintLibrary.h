// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "StableDiffusionSubsystem.h"
#include "DependencySettings.h"
#include "Engine/SceneCapture.h"
#include "GeometryScript/GeometryScriptTypes.h"
#include "GeometryScript/GeometryScriptSelectionTypes.h"
#include "UDynamicMesh.h"
#include "Materials/MaterialInstanceConstant.h"
#include "Layers/LayersSubsystem.h"
#include "ProjectionBakeSession.h"
#include "StableDiffusionToolsSettings.h"
#include "StableDiffusionBlueprintLibrary.generated.h"

/**
 * 
 */
UCLASS()
class STABLEDIFFUSIONTOOLS_API UStableDiffusionBlueprintLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable, Category = "StableDiffusion|Subsystem")
	static UStableDiffusionSubsystem* GetStableDiffusionSubsystem();

	UFUNCTION(BlueprintCallable, Category = "StableDiffusion|Subsystem")
	static ULayersSubsystem* GetLayersSubsystem();

	UFUNCTION(BlueprintCallable, Category = "StableDiffusion|Options")
	static UStableDiffusionToolsSettings* GetPluginOptions();

	UFUNCTION(BlueprintCallable, Category = "StableDiffusion|Options")
	static UDependencySettings* GetDependencyOptions();

	UFUNCTION(BlueprintCallable, Category = "Editor")
	static void RestartEditor();

	UFUNCTION(BlueprintCallable, Category = "Camera")
	static FVector2D ProjectSceneCaptureWorldToUV(const FVector& WorldPosition, USceneCaptureComponent2D* SceneCapture, bool& BehindCamera);

	UFUNCTION(BlueprintCallable, Category = "Camera")
	static FVector2D ProjectViewportWorldToUV(const FVector& WorldPosition, bool& BehindCamera);

	UFUNCTION(BlueprintCallable, Category = "Camera")
	static FTransform GetEditorViewportCameraTransform();

	UFUNCTION(BlueprintCallable, Category = "Camera")
	static FMatrix GetEditorViewportViewProjectionMatrix();

	UFUNCTION(BlueprintCallable, Category = "Camera")
	static FMatrix GetEditorViewportViewMatrix();

	UFUNCTION(BlueprintCallable, Category = "Camera")
	static FMinimalViewInfo GetEditorViewportViewInfo();

	UFUNCTION(BlueprintCallable, Category = "Camera")
	static FIntPoint GetEditorViewportSize();

	UFUNCTION(BlueprintCallable, Category = "Camera")
	static FVector GetEditorViewportDirection();

	UFUNCTION(BlueprintCallable, Category = "Camera")
	static void SetEditorViewportRealtimeOverride(bool Realtime);

	UFUNCTION(BlueprintCallable, Category = "Camera")
	static void ClearEditorViewportRealtimeOverride();

	UFUNCTION(BlueprintCallable, Category = "Camera")
	static bool GetEditorViewportRealtime();

	UFUNCTION(BlueprintCallable, Category = "Camera")
	static TArray<AActor*> GetActorsInViewFrustum(const UObject* WorldContextObject, const FMatrix& ViewProjectionMatrix, const FVector& CameraLocation);
	
	UFUNCTION(BlueprintCallable, Category = "Texture")
	static void CopyTextureDataUsingUVs(UTexture2D* SourceTexture, UTexture2D* TargetTexture, const FIntPoint& ScreenSize, const FMatrix& ViewProjectionMatrix, UDynamicMesh* SourceMesh, const TArray<int> TriangleIDs, bool ClearCoverageMask);

	UFUNCTION(BlueprintCallable, Category = "Texture")
	static FColor GetUVPixelFromTexture(UTexture2D* Texture, FVector2D UV);

	UFUNCTION(BlueprintCallable, Category = "Texture")
	static UTexture2D* CreateTransientTexture(int32 InSizeX, int32 InSizeY, EPixelFormat InFormat = PF_B8G8R8A8, const FName InName = NAME_None);

	UFUNCTION(BlueprintCallable, Category = "Texture")
	static UTexture2D* ColorBufferToTexture(const TArray<FColor>& FrameColors, const FIntPoint& FrameSize, UTexture2D* OutTexture, bool DeferUpdate = false);

	UFUNCTION(BlueprintCallable, Category = "Texture")
	static TArray<FColor> ReadPixels(UTexture* Texture);

	UFUNCTION(BlueprintCallable, Category = "Texture")
	static void UpdateTextureSync(UTexture* Texture);

	UFUNCTION(BlueprintCallable, Category = "Texture")
	static UTexture2D* CreateTextureAsset(const FString& AssetPath, const FString& Name, FIntPoint Size, ETextureSourceFormat Format = ETextureSourceFormat::TSF_BGRA8, FColor Fill = FColor::Black);

	UFUNCTION(BlueprintCallable, Category = "Asset")
	static UStableDiffusionImageResultAsset* CreateImageResultAsset(const FString& PackagePath, const FString& Name, UTexture2D* Texture, FIntPoint Size, const FStableDiffusionImageResult& ImageResult, FMinimalViewInfo View, bool Upsampled = false);

	UFUNCTION(BlueprintCallable, Category = "Asset")
	static UStableDiffusionStyleModelAsset* CreateModelAsset(const FString& PackagePath, const FString& Name);

	UFUNCTION(BlueprintCallable, meta=(DisplayName = "Create LORA Asset", Category = "Asset"))
	static UStableDiffusionLORAAsset* CreateLORAAsset(const FString& PackagePath, const FString& Name);

	UFUNCTION(BlueprintCallable, meta = (DisplayName = "Create Textual Inversion Asset", Category = "Asset"))
	static UStableDiffusionTextualInversionAsset* CreateTextualInversionAsset(const FString& PackagePath, const FString& Name);

	UFUNCTION(BlueprintCallable, Category = "Texture")
	static UProjectionBakeSessionAsset* CreateProjectionBakeSessionAsset(const FProjectionBakeSession& Session, const FString& AssetPath, const FString& Name);

	UFUNCTION(BlueprintCallable, Category = "Texture")
	static FColor LerpColor(const FColor& ColorA, const FColor& ColorB, float Alpha);

	UFUNCTION(BlueprintCallable, Category = "Texture")
	static UMaterialInstanceConstant* CreateMaterialInstanceAsset(UMaterial* ParentMaterial, const FString& Path, const FString& Name);

	static UTexture2D* ColorBufferToTexture(const uint8* FrameData, const FIntPoint& FrameSize, UTexture2D* OutTex, bool DeferUpdate = false);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Texture")
	static FString LayerTypeToString(ELayerImageType LayerType);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Texture")
	static ELayerImageType StringToLayerType(FString LayerName);

	UFUNCTION(BlueprintCallable, Category = "Utilities")
	static UObject* DeepCopyObject(UObject* ObjectToCopy);

private:
	static FEditorViewportClient* GetEditorClient();
	static FSceneView* CalculateEditorView(FSceneViewport* EditorViewport);
};
