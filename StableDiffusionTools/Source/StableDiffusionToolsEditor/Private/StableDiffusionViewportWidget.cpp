#include "StableDiffusionViewportWidget.h"
#include "StableDiffusionBridge.h"
#include "StableDiffusionSubsystem.h"
#include "Components/Image.h"
#include "LevelEditor.h"


void UStableDiffusionViewportWidget::NativeConstruct() {
	Super::NativeConstruct();

	AssetDropTarget->SetContent(
		SNew(SAssetDropTarget)
		.OnAreAssetsAcceptableForDrop_Lambda([](TArrayView<FAssetData> DraggedAssets) {
			for (auto Asset : DraggedAssets) {
				if (Asset.IsInstanceOf(UTexture2D::StaticClass()) || Asset.IsInstanceOf(UStableDiffusionImageResultAsset::StaticClass())) {
					return true;
				}
			}
			return false;
		})
		.OnAssetsDropped_Lambda([this](const FDragDropEvent& DragDropEvent, TArrayView<FAssetData> DroppedAssetData) {
			for (auto Asset : DroppedAssetData) {
				if (Asset.IsInstanceOf(UTexture2D::StaticClass())) {
					ViewportImage->SetBrushFromTexture(Cast<UTexture2D>(Asset.GetAsset()));
				}
				else if (Asset.IsInstanceOf(UStableDiffusionImageResultAsset::StaticClass())) {
					if (auto ImageResult = Cast<UStableDiffusionImageResultAsset>(Asset.GetAsset())) {
						ViewportImage->SetBrushFromTexture(ImageResult->ImageOutput);
					}
				}
			}
	}));
}

bool UStableDiffusionViewportWidget::OnAreAssetsValidForDrop(TArrayView<FAssetData> DraggedAssets) const {
	for (auto Asset : DraggedAssets) {
		if (Asset.IsInstanceOf(UTexture2D::StaticClass()) || Asset.IsInstanceOf(UStableDiffusionImageResultAsset::StaticClass())) {
			return true;
		}
	}
	return false;
}

void UStableDiffusionViewportWidget::HandlePlacementDropped(const FDragDropEvent& DragDropEvent, TArrayView<FAssetData> DroppedAssetData) {
	for (auto Asset : DroppedAssetData) {
		if (Asset.IsInstanceOf(UTexture2D::StaticClass())) {
			ViewportImage->SetBrushFromTexture(Cast<UTexture2D>(Asset.GetAsset()));
		}
		else if (Asset.IsInstanceOf(UStableDiffusionImageResultAsset::StaticClass())) {
			if (auto ImageResult = Cast<UStableDiffusionImageResultAsset>(Asset.GetAsset())) {
				ViewportImage->SetBrushFromTexture(ImageResult->ImageOutput);
			}
		}
	}
}