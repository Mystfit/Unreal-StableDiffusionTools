// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "UDynamicMesh.h"
#include "ProjectionBakeSession.generated.h"


USTRUCT(BlueprintType)
struct STABLEDIFFUSIONTOOLS_API FProjectedMeshTexture 
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projection session")
	UTexture2D* SourceTexture;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projection session")
	FMinimalViewInfo View;
};


USTRUCT(BlueprintType)
struct STABLEDIFFUSIONTOOLS_API FProjectedMeshProperties 
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projection session")
	AActor* Actor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projection session")
	UMeshComponent* MeshComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projection session")
	UDynamicMesh* PreviewMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projection session")
	TArray<FProjectedMeshTexture> SourceTextures;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projection session")
	UTexture2D* TargetTexture;

};


USTRUCT(BlueprintType)
struct STABLEDIFFUSIONTOOLS_API FProjectionBakeSession
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projection session")
	TArray<FProjectedMeshProperties> ProjectedMeshes;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projection session")
	FIntPoint TargetTextureSize;
};


UCLASS(Blueprintable)
class STABLEDIFFUSIONTOOLS_API UProjectionBakeSessionAsset : public UObject
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projection session")
	FProjectionBakeSession Session;
};
