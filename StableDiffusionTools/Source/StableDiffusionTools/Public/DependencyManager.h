#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FDependencyInstallStatus, int, StatusCode, FString, Log);

USTRUCT()
struct FDependencyStatus{
    FName Name;
    bool Installed;
    FString Message;
    FString Version;
}

UCLASS()
class STABLEDIFFUSIONTOOLS_API UDependencyManager : public UObject
{
	GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "StableDiffusion|Dependencies")
    void InstallAllDependencies(bool ForceReinstall);

    UFUNCTION(BlueprintCallable, Category = "StableDiffusion|Dependencies")
    void InstallDependency(FName Dependency);

    UFUNCTION(BlueprintCallable, Category = "StableDiffusion|Dependencies")
    TArray<FName> GetDependencyNames();

    UFUNCTION(BlueprintCallable, Category = "StableDiffusion|Dependencies")
    FDependencyStatus GetDependencyStatus(FName Dependency);

    FDependencyInstallStatus OnDependencyInstalled;
];