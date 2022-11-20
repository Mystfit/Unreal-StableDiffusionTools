#include "StableDiffusionToolsSettings.h"

TSubclassOf<UStableDiffusionBridge> UStableDiffusionToolsSettings::GetGeneratorType() const {
	return GeneratorType;
}
