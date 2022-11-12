#include "DependencyManager.h"
#include "Async/ASync.h"


void UDependencyManager::UpdateDependencyProgress(FString Dependency, FString Line)
{
	AsyncTask(ENamedThreads::GameThread, [this, Dependency, Line]() {
		OnDependencyProgress.Broadcast(Dependency, Line);
	});
}
