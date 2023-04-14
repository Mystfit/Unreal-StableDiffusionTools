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

	UFUNCTION(BlueprintCallable, Category = "StableDiffusion|Options")
	static UStableDiffusionToolsSettings* GetPluginOptions();

	UFUNCTION(BlueprintCallable, Category = "StableDiffusion|Options")
	static UDependencySettings* GetDependencyOptions();

	UFUNCTION(BlueprintCallable, Category = "StableDiffusion")
	static void RestartEditor();

	UFUNCTION(BlueprintCallable, Category = "Camera")
	static FVector2D ProjectWorldToEditorViewportUV(const FVector& WorldPosition);

	UFUNCTION(BlueprintCallable, Category = "Camera")
	static FMatrix GetEditorViewportViewProjectionMatrix();

	UFUNCTION(BlueprintCallable, Category = "Camera")
	static FVector GetEditorViewportDirection();

	UFUNCTION(BlueprintCallable, Category = "Camera")
	static TArray<AActor*> GetActorsInViewFrustum(const UObject* WorldContextObject, const FMatrix& ViewProjectionMatrix);

	/**
	* Scale of PlaneTransform defines world-space dimension that maps to 1 UV dimension
	*/
	UFUNCTION(BlueprintCallable, Category = "GeometryScript|UVs", meta = (ScriptMethod))
		static UPARAM(DisplayName = "Target Mesh") UDynamicMesh*
		SetMeshUVsFromViewProjection(
			UDynamicMesh* TargetMesh,
			int UVSetIndex,
			FMatrix ViewProjMatrix,
			FGeometryScriptMeshSelection Selection,
			bool FrontFacingOnly = true,
			UGeometryScriptDebug* Debug = nullptr);
};
