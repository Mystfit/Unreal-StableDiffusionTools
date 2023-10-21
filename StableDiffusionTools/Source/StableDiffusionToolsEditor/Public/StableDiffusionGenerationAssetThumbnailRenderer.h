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
		UTexture2D* Texture = nullptr;
		FStableDiffusionPipelineImageResult ImageResult;
		if (Asset){
			Asset->GetLastValidStageResult(ImageResult);
			Texture = ImageResult.ImageOutput.OutTexture;
		}
		return Texture != nullptr ? UTextureThumbnailRenderer::CanVisualizeAsset(Texture) : false;
	}

	void Draw(UObject* Object, int32 X, int32 Y, uint32 Width, uint32 Height, FRenderTarget* RenderTarget, FCanvas* Canvas, bool bAdditionalViewFamily)
	{
		UStableDiffusionImageResultAsset* Asset = Cast<UStableDiffusionImageResultAsset>(Object);
		FStableDiffusionPipelineImageResult ImageResult;
		Asset->GetLastValidStageResult(ImageResult);
		if (ImageResult.ImageOutput.OutTexture)
		{
			UTextureThumbnailRenderer::Draw(ImageResult.ImageOutput.OutTexture, X, Y, Width, Height, RenderTarget, Canvas, bAdditionalViewFamily);
		}
	}
};
