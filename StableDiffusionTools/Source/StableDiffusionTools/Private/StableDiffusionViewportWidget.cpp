#include "StableDiffusionViewportWidget.h"
#include "StableDiffusionBridge.h"
#include "StableDiffusionSubsystem.h"
#include "Components/Image.h"
#include "..\Public\StableDiffusionViewportWidget.h"

UStableDiffusionSubsystem* UStableDiffusionViewportWidget::GetStableDiffusionSubsystem() {
	UStableDiffusionSubsystem* subsystem = nullptr;
	subsystem = GEditor->GetEditorSubsystem<UStableDiffusionSubsystem>();
	return subsystem;
}

void UStableDiffusionViewportWidget::UpdateViewportImage(const FString& Prompt, FIntPoint Size, const TArray<FColor>& PixelData)
{
	auto BrushTex = Cast<UTexture2D>(ViewportImage->Brush.GetResourceObject());
	if (BrushTex) {
		auto subsystem = GEditor->GetEditorSubsystem<UStableDiffusionSubsystem>();
		if (subsystem) {
			subsystem->ColorBufferToTexture(Prompt, (uint8*)PixelData.GetData(), Size, BrushTex);
			ViewportImage->SetBrushFromTexture(BrushTex);
		}
	}
}
