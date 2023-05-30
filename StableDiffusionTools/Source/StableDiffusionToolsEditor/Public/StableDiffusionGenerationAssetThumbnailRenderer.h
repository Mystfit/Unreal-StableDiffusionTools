#pragma once

#include "StableDiffusionImageResult.h"
#include "ThumbnailRendering/ThumbnailRenderer.h"
#include "ThumbnailRendering/TextureThumbnailRenderer.h"
#include "StableDiffusionGenerationAssetThumbnailRenderer.generated.h"

UCLASS()
class UStableDiffusionGenerationAssetThumbnailRenderer : public UTextureThumbnailRenderer
{
	GENERATED_BODY()

public:
	bool CanVisualizeAsset(UObject* Object)
	{
		UStableDiffusionImageResultAsset* Asset = Cast<UStableDiffusionImageResultAsset>(Object);
		UTexture2D* Texture = Asset != nullptr ? Asset->ImageOutput.OutTexture : nullptr;
		return Texture != nullptr ? UTextureThumbnailRenderer::CanVisualizeAsset(Texture) : false;
	}

	void Draw(UObject* Object, int32 X, int32 Y, uint32 Width, uint32 Height, FRenderTarget* RenderTarget, FCanvas* Canvas, bool bAdditionalViewFamily)
	{
		UStableDiffusionImageResultAsset* Asset = Cast<UStableDiffusionImageResultAsset>(Object);
		UTexture2D* Texture = Asset != nullptr ? Asset->ImageOutput.OutTexture : nullptr;
		if (Texture != nullptr)
		{
			UTextureThumbnailRenderer::Draw(Texture, X, Y, Width, Height, RenderTarget, Canvas, bAdditionalViewFamily);
		}
	}
};
