#pragma once

#include "AssetTypeActions_Base.h"

class FStableDiffusionImageResultAssetActions : public FAssetTypeActions_Base
{
public:
	virtual UClass* GetSupportedClass() const override { return UStableDiffusionImageResultAsset::StaticClass(); }
	virtual FText GetName() const { return FText::FromString("Diffusion Image Result"); };
	virtual FColor GetTypeColor() const { return FColor::Orange; }
	virtual uint32 GetCategories() { return EAssetTypeCategories::Textures; }
};
