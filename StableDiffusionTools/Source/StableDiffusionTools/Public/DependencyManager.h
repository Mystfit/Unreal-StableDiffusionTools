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


USTRUCT(BlueprintType)
struct STABLEDIFFUSIONTOOLS_API FDependencyManifestEntry {
    GENERATED_BODY()
public:
    UPROPERTY(BlueprintReadWrite)
        FString Name;

    UPROPERTY(BlueprintReadWrite)
        FString Module;

    UPROPERTY(BlueprintReadWrite)
        FString Args;

    UPROPERTY(BlueprintReadWrite)
        FString Url;

    UPROPERTY(BlueprintReadWrite)
        FString Version;

    UPROPERTY(BlueprintReadWrite)
        bool Upgrade;
};


USTRUCT(BlueprintType)
struct STABLEDIFFUSIONTOOLS_API FDependencyManifest {
    GENERATED_BODY()
public:
    UPROPERTY(BlueprintReadWrite)
        TArray<FDependencyManifestEntry> ManifestEntries;
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
        FDependencyStatus InstallDependency(FDependencyManifestEntry Dependency, bool ForceReinstall);

    UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category = "StableDiffusion|Dependencies")
        TArray<FName> GetDependencyNames();

    UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category = "StableDiffusion|Dependencies")
        FDependencyStatus GetDependencyStatus(FDependencyManifestEntry Dependency);

    UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category = "StableDiffusion|Dependencies")
        bool AllDependenciesInstalled();
    
    UPROPERTY(BlueprintAssignable, Category = "StableDiffusion|Dependencies")
    FDependencyInstallStatus OnDependencyInstalled;

    UPROPERTY(BlueprintAssignable, Category = "StableDiffusion|Dependencies")
    FDependencyInstallProgress OnDependencyProgress;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "StableDiffusion|Dependencies")
    TMap<FString, FDependencyManifest > DependencyManifests;

    //FDependencyInstallProgress OnDependencyProgress;
protected:
    UFUNCTION(BlueprintCallable, Category = "StableDiffusion|Dependencies")
    void UpdateDependencyProgress(FString DependencyName, FString Line);

};