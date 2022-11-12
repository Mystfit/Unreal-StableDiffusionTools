// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "EditorUtilityWidget.h"
#include "SDDependencyInstallerWidget.generated.h"

/**
 * 
 */
UCLASS()
class STABLEDIFFUSIONTOOLSEDITOR_API USDDependencyInstallerWidget : public UEditorUtilityWidget
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable, Category = "StableDiffusion|Subsystem")
	UStableDiffusionSubsystem* GetStableDiffusionSubsystem();
};
