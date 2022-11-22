// Fill out your copyright notice in the Description page of Project Settings.
#include "StableDiffusionBridge.h"
#include "Math/Color.h"
#include "Async/Async.h"
#include "StableDiffusionToolsSettings.h"

UStableDiffusionBridge::UStableDiffusionBridge(const FObjectInitializer& initializer) : Super(initializer) {
    //CachedToken = initializer.CreateDefaultSubobject<USDBridgeToken>(this, FName(TEXT("CachedToken")));
}

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

bool UStableDiffusionBridge::LoginUsingToken(const FString& Token)
{
    return true;
}

FString UStableDiffusionBridge::GetToken()
{
    const UStableDiffusionToolsSettings* Settings = GetDefault<UStableDiffusionToolsSettings>();

    auto Tokens = Settings->GetGeneratorTokens();
    auto Token = Tokens.Find(GetClass()->GetFName());
    if (Token) {
        if(Token->IsEmpty())
            UE_LOG(LogTemp, Warning, TEXT("Generator %s token is empty. Please set the token in Project Settings->Stable Diffusion Tools->Generator Tokens"), *GetClass()->GetFName().ToString())
        return *Token;
    }
    UE_LOG(LogTemp, Warning, TEXT("No token found for generator %s"), *GetClass()->GetFName().ToString())
    return "";
}

void UStableDiffusionBridge::UpdateImageProgress(FString prompt, int32 step, int32 timestep, int32 width, int32 height, const TArray<FColor>& FrameColors)
{
	AsyncTask(ENamedThreads::GameThread, [this, prompt, step, timestep, width, height, FrameColors] {
		OnImageProgress.Broadcast(step, timestep, FIntPoint(width, height), FrameColors);
		OnImageProgressEx.Broadcast(step, timestep, FIntPoint(width, height), FrameColors);
	});
}

void UStableDiffusionBridge::SaveProperties()
{
    //CachedToken->SaveConfig();
}
