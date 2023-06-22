// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "Interfaces/IHttpRequest.h"
#include "Kismet/BlueprintAsyncActionBase.h"

#include "ModelAssetTools.generated.h"

class UTexture2DDynamic;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FDownloadFileDelegate, FString, FilePath, int64, BytesDownloadedSinceLastUpdate);

UCLASS()
class STABLEDIFFUSIONTOOLS_API UAsyncTaskDownloadModel : public UBlueprintAsyncActionBase
{
	GENERATED_UCLASS_BODY()

public:
	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true"))
		static UAsyncTaskDownloadModel* DownloadModel(FString URL, FString Path);

public:

	UPROPERTY(BlueprintAssignable)
		FDownloadFileDelegate OnSuccess;

	UPROPERTY(BlueprintAssignable)
		FDownloadFileDelegate OnUpdate;

	UPROPERTY(BlueprintAssignable)
		FDownloadFileDelegate OnFail;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
		FString SavePath;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
		int64 TotalBytesDownloaded = 0;

public:

	void Start(FString URL);

protected:

	/** Handles HTTP requests coming from the web */
	//UFUNCTION(BlueprintNativeImplementable, category = "HTTP")
	void HandleHTTPRequest(FHttpRequestPtr HttpRequest, FHttpResponsePtr HttpResponse, bool bSucceeded);

	/* Handles progress updates for the HTTP request */
	//UFUNCTION(BlueprintNativeImplementable, category = "HTTP")
	void HandleHTTPProgress(FHttpRequestPtr Request, int32 TotalBytesWritten, int32 BytesWrittenSinceLastUpdate);

};
