// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "DependencySettings.generated.h"

/**
 * 
 */
UCLASS(config = Engine, defaultconfig)
class STABLEDIFFUSIONTOOLS_API UDependencySettings : public UObject
{
	GENERATED_BODY()
public:
	/**
	* Automatically load all bridge scripts when the editor starts. Disable this if you are having issues with updating python bridge script dependencies that are loaded into memory.
	**/
	UPROPERTY(BlueprintReadWrite, config, EditAnywhere, Category = "Options")
		bool AutoLoadBridgeScripts = true;

	/**
	* Automatically load all bridge scripts when the editor starts. Disable this if you are having issues with updating python bridge script dependencies that are loaded into memory.
	**/
	UPROPERTY(BlueprintReadWrite, config, EditAnywhere, Category = "Options")
		bool AutoUpdateDependenciesOnStartup = false;

	/**
	* Remove all downloaded python dependencies when the editor next starts.
	**/
	UPROPERTY(BlueprintReadWrite, config, EditAnywhere, Category = "Options")
		bool ClearDependenciesOnEditorRestart = false;

	/**
	* Remove all downloaded python dependencies from Unreal's site-packages folder when the editor next starts.
	**/
	UPROPERTY(BlueprintReadWrite, config, EditAnywhere, Category = "Options")
		bool ClearSystemDependenciesOnEditorRestart = false;
};
