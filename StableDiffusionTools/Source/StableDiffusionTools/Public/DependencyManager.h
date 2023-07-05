#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "DependencyManager.generated.h"

UENUM(BlueprintType)
enum class EDependencyState : uint8 {
    NotInstalled UMETA(DisplayName = "Not installed"),
    Installed UMETA(DisplayName = "Installed"),
    Updating UMETA(DisplayName = "Updating"),
    Queued UMETA(DisplayName = "Queued"),
    Error UMETA(DisplayName = "Error")
};
ENUM_CLASS_FLAGS(EDependencyState);

USTRUCT(BlueprintType)
struct STABLEDIFFUSIONTOOLS_API FDependencyStatus {
    GENERATED_BODY()
public:
    UPROPERTY(BlueprintReadWrite, Category = "StableDiffusion|Dependencies")
        FName Name;

    UPROPERTY(BlueprintReadWrite, Category = "StableDiffusion|Dependencies")
        EDependencyState Status;

    UPROPERTY(BlueprintReadWrite, Category = "StableDiffusion|Dependencies")
        FString Message;

    UPROPERTY(BlueprintReadWrite, Category = "StableDiffusion|Dependencies")
        FString Version;

    UPROPERTY(BlueprintReadWrite, Category = "StableDiffusion|Dependencies")
        int ReturnCode;
};


USTRUCT(BlueprintType)
struct STABLEDIFFUSIONTOOLS_API FDependencyManifestEntry {
    GENERATED_BODY()
public:
    UPROPERTY(BlueprintReadWrite, Category = "StableDiffusion|Dependencies")
        FString Name;

    UPROPERTY(BlueprintReadWrite, Category = "StableDiffusion|Dependencies")
        FString Module;

    UPROPERTY(BlueprintReadWrite, Category = "StableDiffusion|Dependencies")
        FString Args;

    UPROPERTY(BlueprintReadWrite, Category = "StableDiffusion|Dependencies")
        FString Url;

    UPROPERTY(BlueprintReadWrite, Category = "StableDiffusion|Dependencies")
        FString Version;

    UPROPERTY(BlueprintReadWrite, Category = "StableDiffusion|Dependencies")
        FString Branch;

    UPROPERTY(BlueprintReadWrite, Category = "StableDiffusion|Dependencies")
        bool Upgrade;

    UPROPERTY(BlueprintReadWrite, Category = "StableDiffusion|Dependencies")
        bool NoCache;
};


USTRUCT(BlueprintType)
struct STABLEDIFFUSIONTOOLS_API FDependencyManifest {
    GENERATED_BODY()
public:
    UPROPERTY(BlueprintReadWrite, Category = "StableDiffusion|Dependencies")
        TArray<FDependencyManifestEntry> ManifestEntries;
};


DECLARE_DYNAMIC_MULTICAST_DELEGATE(FPythonReady);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FDependencyInstallStatus, FDependencyStatus, Status);
//DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FDependencyInstallProgress, FString, Name, FString, Line);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FDependencyInstallProgress, FString, Name, FString, Line);
//DECLARE_MULTICAST_DELEGATE_TwoParams(FDependencyInstallProgress, FString, FString);


UCLASS()
class STABLEDIFFUSIONTOOLS_API UDependencyManager : public UObject
{
    GENERATED_BODY()
public:
    UDependencyManager(const FObjectInitializer& initializer);

    UFUNCTION(BlueprintCallable, Category = "StableDiffusion|Dependencies")
        void SetIsInstallingDependencies(bool State);

    UFUNCTION(BlueprintCallable, Category = "StableDiffusion|Dependencies")
        bool IsInstallingDependencies() const;

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

    UFUNCTION(BlueprintCallable, Category = "StableDiffusion|Dependencies")
        void RestartAndUpdateDependencies();

    UFUNCTION(BlueprintCallable, Category = "StableDiffusion|Dependencies")
        void ResetDependencies(bool ClearSystemDeps = false);

    UFUNCTION(BlueprintCallable, Category = "StableDiffusion|Dependencies")
        void FinishedClearingDependencies();

    UFUNCTION(BlueprintCallable, Category = "StableDiffusion|Dependencies")
        void FinishedUpdatingDependencies();

    UFUNCTION(BlueprintCallable, Category = "StableDiffusion|Dependencies")
        FString GetPluginVersionName();
    
    UPROPERTY(BlueprintAssignable, Category = "StableDiffusion|Dependencies")
    FDependencyInstallStatus OnDependencyInstalled;

    UPROPERTY(BlueprintAssignable, Category = "StableDiffusion|Dependencies")
    FDependencyInstallProgress OnDependencyProgress;

    UPROPERTY(BlueprintAssignable, Category = "StableDiffusion|Dependencies")
    FPythonReady OnPythonReady;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "StableDiffusion|Dependencies")
    TMap<FString, FDependencyManifest > DependencyManifests;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "StableDiffusion|Dependencies")
    FString PluginSitePackages;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "StableDiffusion|Dependencies")
    bool FrozenDependenciesAvailable;

    //FDependencyInstallProgress OnDependencyProgress;
protected:
    UFUNCTION(BlueprintCallable, Category = "StableDiffusion|Dependencies")
    void UpdateDependencyProgress(FString DependencyName, FString Line);

    bool bIsInstallingDependencies = false;
};