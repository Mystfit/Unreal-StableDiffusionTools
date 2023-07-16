// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "EditorUtilityWidget.h"
#include "Blueprint/IUserObjectListEntry.h"
#include "StableDiffusionGenerationOptions.h"
#include "ImagePipelineStageWidget.generated.h"

/**
 * 
 */
UCLASS()
class STABLEDIFFUSIONTOOLSEDITOR_API UImagePipelineStageWidget : 
	public UEditorUtilityWidget,
	public IUserObjectListEntry
{
	GENERATED_BODY()
public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Instanced, Category = "Image pipeline")
	UImagePipelineStageAsset* PipelineStage;
};
