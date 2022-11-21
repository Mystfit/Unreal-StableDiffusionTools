// Fill out your copyright notice in the Description page of Project Settings.
#include "StableDiffusionBridge.h"
#include "Math/Color.h"
#include "Async/Async.h"

UStableDiffusionBridge* UStableDiffusionBridge::Get()
{
    TArray<UClass*> PythonBridgeClasses;
    GetDerivedClasses(UStableDiffusionBridge::StaticClass(), PythonBridgeClasses);
    int32 NumClasses = PythonBridgeClasses.Num();
    if (NumClasses > 0)
        return Cast<UStableDiffusionBridge>(PythonBridgeClasses[NumClasses - 1]->GetDefaultObject());
    {
    }
    return nullptr;
};

void UStableDiffusionBridge::UpdateImageProgress(FString prompt, int32 step, int32 timestep, int32 width, int32 height, const TArray<FColor>& FrameColors)
{
	AsyncTask(ENamedThreads::GameThread, [this, prompt, step, timestep, width, height, FrameColors] {
		OnImageProgress.Broadcast(step, timestep, FIntPoint(width, height), FrameColors);
		OnImageProgressEx.Broadcast(step, timestep, FIntPoint(width, height), FrameColors);
	});
}

void UStableDiffusionBridge::SaveProperties()
{
    this->SaveConfig();
}
