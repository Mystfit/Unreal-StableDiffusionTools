// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "OpenPoseInterface.generated.h"

/**
 * 
 */
UINTERFACE(MinimalAPI, Blueprintable)
class UOpenPoseInterface : public UInterface
{
	GENERATED_BODY()
public:

};

class IOpenPoseInterface : public UInterface
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category = Pose)
	void SetPoseVisible(bool Visible);

	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category = Pose)
	bool IsPoseVisible();

	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category = Pose)
	void SetJointSize(float Size);

	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category = Pose)
	void SetConnectorSize(float Size);
};
