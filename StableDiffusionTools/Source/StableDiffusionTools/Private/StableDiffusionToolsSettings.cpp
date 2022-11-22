#include "StableDiffusionToolsSettings.h"

TSubclassOf<UStableDiffusionBridge> UStableDiffusionToolsSettings::GetGeneratorType() const {
	return GeneratorType;
}

TMap<FName, FString> UStableDiffusionToolsSettings::GetGeneratorTokens() const {
	return GeneratorTokens;
}

void UStableDiffusionToolsSettings::AddGeneratorToken(const FName& Generator)
{
	if (!GeneratorTokens.Contains(Generator)) {
		GeneratorTokens.Add(Generator);
	}
}
