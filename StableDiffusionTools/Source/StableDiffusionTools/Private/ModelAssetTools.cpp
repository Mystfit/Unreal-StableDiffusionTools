// Copyright Epic Games, Inc. All Rights Reserved.

#include "ModelAssetTools.h"
#include "Modules/ModuleManager.h"
#include "Misc/FileHelper.h"
#include "IImageWrapperModule.h"
#include "Interfaces/IHttpResponse.h"
#include "HttpModule.h"
#include "Async/Async.h"
#include "IPythonScriptPlugin.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ModelAssetTools)


//----------------------------------------------------------------------//
// UAsyncTaskDownloadModel
//----------------------------------------------------------------------//

UAsyncTaskDownloadModel::UAsyncTaskDownloadModel(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	if (HasAnyFlags(RF_ClassDefaultObject) == false)
	{
		AddToRoot();
	}
}

UAsyncTaskDownloadModel* UAsyncTaskDownloadModel::DownloadModelCURL(FString URL, FString Path)
{
	UAsyncTaskDownloadModel* DownloadTask = NewObject<UAsyncTaskDownloadModel>();
	DownloadTask->SavePath = Path;
	DownloadTask->Start(URL);

	return DownloadTask;
}

void UAsyncTaskDownloadModel::SuccessGameThread(int64 TotalBytes)
{
	AsyncTask(ENamedThreads::GameThread, [this, TotalBytes]() {
		OnSuccess.Broadcast(SavePath, TotalBytes);
	});
}

void UAsyncTaskDownloadModel::UpdateGameThread(int64 TotalBytes)
{
	AsyncTask(ENamedThreads::GameThread, [this, TotalBytes]() {
		OnUpdate.Broadcast("", TotalBytes);
	});
}

void UAsyncTaskDownloadModel::FailGameThread(int64 TotalBytes)
{
	AsyncTask(ENamedThreads::GameThread, [this, TotalBytes]() {
		OnFail.Broadcast("", TotalBytes);
	});
}

void UAsyncTaskDownloadModel::Start(FString URL)
{
#if !UE_SERVER
	// Create the Http request and add to pending request list
	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> HttpRequest = FHttpModule::Get().CreateRequest();

	HttpRequest->OnProcessRequestComplete().BindUObject(this, &UAsyncTaskDownloadModel::HandleHTTPRequest);
	HttpRequest->OnRequestProgress().BindUObject(this, &UAsyncTaskDownloadModel::HandleHTTPProgress);
	HttpRequest->SetURL(URL);
	HttpRequest->SetVerb(TEXT("GET"));
	HttpRequest->ProcessRequest();
#else
	// On the server we don't execute fail or success we just don't fire the request.
	RemoveFromRoot();
#endif
}

void UAsyncTaskDownloadModel::HandleHTTPProgress(FHttpRequestPtr Request, int32 TotalBytesWritten, int32 BytesWrittenSinceLastUpdate)
{
	// Don't send download updates too frequently otherwise we'll spam the blueprint thread
	if (BytesWrittenSinceLastUpdate - TotalBytesDownloaded > 1000000) {
		OnUpdate.Broadcast("", BytesWrittenSinceLastUpdate);
		TotalBytesDownloaded = BytesWrittenSinceLastUpdate;
	}
}

void UAsyncTaskDownloadModel::HandleHTTPRequest(FHttpRequestPtr HttpRequest, FHttpResponsePtr HttpResponse, bool bSucceeded)
{
#if !UE_SERVER

	RemoveFromRoot();

	if (bSucceeded && HttpResponse.IsValid() && HttpResponse->GetContentLength() > 0)
	{
		TArray<uint8> ResponseData = HttpResponse->GetContent();

		// Get filename from Content-Disposition header
		auto ContentDispHeader = HttpResponse->GetHeader(TEXT("Content-Disposition"));
		if (ContentDispHeader.IsEmpty())
		{
			UE_LOG(LogTemp, Error, TEXT("Failed to download file, no Content-Disposition header found"));
			OnFail.Broadcast("", ResponseData.Num());
			return;
		}
		TArray<FString> ContentDisposition;
		ContentDispHeader.ParseIntoArray(ContentDisposition, TEXT(";"), true);
		auto Filename = ContentDisposition.FindByPredicate([](const FString& In) { 
			return In.Contains(TEXT("filename="));
		});

		if (Filename) {
			FString SanitizedFilename = Filename->Replace(TEXT("filename="), TEXT("")).Replace(TEXT("\""), TEXT("")).TrimStartAndEnd();
			// Save file content to disk
			
			FString FullSavePath = FPaths::Combine(*SavePath, *SanitizedFilename);

			if (FFileHelper::SaveArrayToFile(ResponseData, *FullSavePath))
			{
				UE_LOG(LogTemp, Warning, TEXT("File downloaded successfully to %s"), *FullSavePath);

				// Call the delegate
				OnSuccess.Broadcast(FullSavePath, ResponseData.Num());
			}
			else
			{
				UE_LOG(LogTemp, Error, TEXT("Failed to save file to %s"), *FullSavePath);
			}
		}
		else {
			UE_LOG(LogTemp, Error, TEXT("Failed to save file. No filename found in HTTP response."));
		}
	}

	OnFail.Broadcast("", 0);

#endif
}


//----------------------------------------------------------------------//

UAsyncOperation::UAsyncOperation(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	if (HasAnyFlags(RF_ClassDefaultObject) == false)
	{
		AddToRoot();
	}
}

UAsyncOperation* UAsyncOperation::StartAsyncOperation()
{
	UAsyncOperation* Operation = NewObject<UAsyncOperation>();

	AsyncTask(ENamedThreads::AnyThread, [Operation]() {
		Operation->OnStart.Broadcast(Operation);
	});

	return Operation;
}

void UAsyncOperation::Complete()
{
#if !UE_SERVER
	RemoveFromRoot();
	AsyncTask(ENamedThreads::GameThread, [this]() {
		OnComplete.Broadcast(this);
	});
#endif

}
