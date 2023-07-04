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

FDirectoryPath UStableDiffusionBridge::GetSettingsModelSavePath()
{
    auto Settings = GetMutableDefault<UStableDiffusionToolsSettings>();
    return Settings->GetModelDownloadPath();
}

void UStableDiffusionBridge::UpdateImageProgress(FString prompt, int32 step, int32 timestep, float progress, int32 width, int32 height, UTexture2D* Texture)
{
    AsyncTask(ENamedThreads::GameThread, [this, prompt, step, timestep, progress, width, height, Texture] {
        if (IsValid(Texture)) {
            Texture->UpdateResource();
            while (!Texture->IsAsyncCacheComplete()) {
                FPlatformProcess::Sleep(0.0f);
            }
#if WITH_EDITOR
            Texture->PostEditChange();
#endif
        }
		OnImageProgress.Broadcast(step, timestep, progress, FIntPoint(width, height), Texture);
		OnImageProgressEx.Broadcast(step, timestep, progress, FIntPoint(width, height), Texture);
	});
}

void UStableDiffusionBridge::SaveProperties()
{
    //CachedToken->SaveConfig();
}
