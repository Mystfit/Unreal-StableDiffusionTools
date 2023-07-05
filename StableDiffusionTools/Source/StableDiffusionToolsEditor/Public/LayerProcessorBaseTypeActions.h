#pragma once

#include "CoreMinimal.h"
#include "LayerProcessorBase.h"
#include "AssetTypeActions_Base.h"

class FAssetTypeActions_LayerProcessorBase : public FAssetTypeActions_Base
{
public:
	// IAssetTypeActions Implementation
	virtual FText GetName() const override { return NSLOCTEXT("AssetTypeActions", "AssetTypeActions_LayerProcessorBase", "Layer Processor"); }
	virtual FColor GetTypeColor() const override { return FColor(12, 65, 12); }
	virtual UClass* GetSupportedClass() const override { return ULayerProcessorBase::StaticClass(); }
	virtual uint32 GetCategories() override { return EAssetTypeCategories::Misc; }
};

