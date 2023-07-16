// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "EditorUtilityWidget.h"
#include "StableDiffusionGenerationOptions.h"
#include "ImagePipelineWidget.generated.h"

/**
 * 
 */
UCLASS()
class STABLEDIFFUSIONTOOLSEDITOR_API UImagePipelineWidget : public UEditorUtilityWidget
{
	GENERATED_BODY()
public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Instanced, Category = "Image pipeline")
	UImagePipelineAsset* PipelineAsset;
};
