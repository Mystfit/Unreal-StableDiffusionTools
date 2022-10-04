#include "StableDiffusionViewportWidget.h"
#include "StableDiffusionBridge.h"
#include "StableDiffusionSubsystem.h"
#include "Components/Image.h"
//
//void UStableDiffusionViewportWidget::NativeConstruct()
//{ 
//	/*Super::NativeConstruct();
//	ViewportImage = CreateWidget<UImage>(this, UImage::StaticClass(), "ViewportImageDisplay");*/
//}
//
//TSharedRef<SWidget> UStableDiffusionViewportWidget::RebuildWidget()
//{ 
//	//ViewportImage = WidgetTree->ConstructWidget<UImage>(this, UImage::StaticClass(), "ViewportImageDisplay");
//}

void UStableDiffusionViewportWidget::InitModel(FIntPoint size)
{
	auto subsystem = GEditor->GetEditorSubsystem<UStableDiffusionSubsystem>();
	if (!subsystem)
		return;

	if(subsystem->GeneratorBridge)
		subsystem->GeneratorBridge->InitModel();

	subsystem->StartCapturingViewport(size);
}


void UStableDiffusionViewportWidget::GenerateImage(const FString& prompt, FIntPoint size, float InputStrength, int32 Iterations, int32 Seed)
{
	//auto Bridge = UStableDiffusionBridge::Get();
	auto subsystem = GEditor->GetEditorSubsystem<UStableDiffusionSubsystem>();
	if (subsystem) {
		subsystem->OnImageGenerationComplete.AddLambda([this](UTexture2D* texture) {
			if (ViewportImage) {
				// TODO: Will this leak when replaced?
				if (texture) {
					DisplayedTexture = texture;
					ViewportImage->SetBrushFromTexture(DisplayedTexture);
				}
			}
		});
		subsystem->GenerateImage(prompt, size, InputStrength, Iterations, Seed);
	}
}
