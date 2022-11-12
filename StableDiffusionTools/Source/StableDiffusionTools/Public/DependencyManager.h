#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "DependencyManager.generated.h"

USTRUCT(BlueprintType)
struct STABLEDIFFUSIONTOOLS_API FDependencyStatus {
    GENERATED_BODY()
public:
    UPROPERTY(BlueprintReadWrite)
        FName Name;

    UPROPERTY(BlueprintReadWrite)
        bool Installed;

    UPROPERTY(BlueprintReadWrite)
        FString Message;

    UPROPERTY(BlueprintReadWrite)
        FString Version;

    UPROPERTY(BlueprintReadWrite)
        int ReturnCode;
};


DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FDependencyInstallStatus, FDependencyStatus, Status);
//DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FDependencyInstallProgress, FString, Name, FString, Line);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FDependencyInstallProgress, FString, Name, FString, Line);
//DECLARE_MULTICAST_DELEGATE_TwoParams(FDependencyInstallProgress, FString, FString);


UCLASS()
class STABLEDIFFUSIONTOOLS_API UDependencyManager : public UObject
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category = "StableDiffusion|Dependencies")
        void InstallAllDependencies(bool ForceReinstall);

    UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category = "StableDiffusion|Dependencies")
        FDependencyStatus InstallDependency(FName Dependency, bool ForceReinstall);

    UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category = "StableDiffusion|Dependencies")
        TArray<FName> GetDependencyNames();

    UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category = "StableDiffusion|Dependencies")
        FDependencyStatus GetDependencyStatus(FName Dependency);

    UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category = "StableDiffusion|Dependencies")
        bool AllDependenciesInstalled();
    
    UPROPERTY(BlueprintAssignable, Category = "StableDiffusion|Dependencies")
    FDependencyInstallStatus OnDependencyInstalled;

    UPROPERTY(BlueprintAssignable, Category = "StableDiffusion|Dependencies")
    FDependencyInstallProgress OnDependencyProgress;

    //FDependencyInstallProgress OnDependencyProgress;
protected:
    UFUNCTION(BlueprintCallable, Category = "StableDiffusion|Dependencies")
    void UpdateDependencyProgress(FString Dependency, FString Line);
};