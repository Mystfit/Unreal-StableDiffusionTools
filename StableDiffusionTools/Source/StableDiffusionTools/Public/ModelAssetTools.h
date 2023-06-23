// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "Interfaces/IHttpRequest.h"
#include "Kismet/BlueprintAsyncActionBase.h"

#include "ModelAssetTools.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FDownloadFileDelegate, FString, FilePath, int64, BytesDownloadedSinceLastUpdate);

UCLASS(Blueprintable)
class STABLEDIFFUSIONTOOLS_API UAsyncTaskDownloadModel : public UObject
{
	GENERATED_UCLASS_BODY()

public:
	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true"))
		static UAsyncTaskDownloadModel* DownloadModelCURL(FString URL, FString Path);

	UFUNCTION(BlueprintCallable)
		void SuccessGameThread(int64 TotalBytes);

	UFUNCTION(BlueprintCallable)
		void UpdateGameThread(int64 TotalBytes);

	UFUNCTION(BlueprintCallable)
		void FailGameThread(int64 TotalBytes);


public:

	UPROPERTY(BlueprintAssignable, BlueprintReadWrite)
		FDownloadFileDelegate OnSuccess;

	UPROPERTY(BlueprintAssignable, BlueprintReadWrite)
		FDownloadFileDelegate OnUpdate;

	UPROPERTY(BlueprintAssignable, BlueprintReadWrite)
		FDownloadFileDelegate OnFail;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
		FString SavePath;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
		int64 TotalBytesDownloaded = 0;

public:

	void Start(FString URL);

private:

	/** Handles HTTP requests coming from the web */
	void HandleHTTPRequest(FHttpRequestPtr HttpRequest, FHttpResponsePtr HttpResponse, bool bSucceeded);

	/* Handles progress updates for the HTTP request */
	void HandleHTTPProgress(FHttpRequestPtr Request, int32 TotalBytesWritten, int32 BytesWrittenSinceLastUpdate);

};


class UAsyncOperation;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FAsyncOperation, UAsyncOperation*, EventOwner);

UCLASS(Blueprintable)
class STABLEDIFFUSIONTOOLS_API UAsyncOperation : public UBlueprintAsyncActionBase
{
	GENERATED_UCLASS_BODY()

public:
	UFUNCTION(BlueprintCallable, meta = (Category = "Async", BlueprintInternalUseOnly = "true"))
		static UAsyncOperation* StartAsyncOperation();

	UFUNCTION(BlueprintCallable, category = Category)
		void Complete();

public:

	UPROPERTY(BlueprintAssignable, BlueprintReadWrite)
		FAsyncOperation OnStart;

	UPROPERTY(BlueprintAssignable, BlueprintReadWrite)
		FAsyncOperation OnComplete;

};
