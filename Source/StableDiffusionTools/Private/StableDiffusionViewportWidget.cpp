#include "StableDiffusionViewportWidget.h"
#include "StableDiffusionBridge.h"
#include "StableDiffusionSubsystem.h"
#include "Components/Image.h"
#include "..\Public\StableDiffusionViewportWidget.h"
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

UStableDiffusionSubsystem* UStableDiffusionViewportWidget::GetStableDiffusionSubsystem() {
	UStableDiffusionSubsystem* subsystem = nullptr;
	subsystem = GEditor->GetEditorSubsystem<UStableDiffusionSubsystem>();
	return subsystem;
}

void UStableDiffusionViewportWidget::InstallDependencies()
{
	auto subsystem = GEditor->GetEditorSubsystem<UStableDiffusionSubsystem>();
	if (!subsystem)
		return;

	subsystem->InstallDependencies();
}

void UStableDiffusionViewportWidget::InitModel(FIntPoint size)
{
	auto subsystem = GEditor->GetEditorSubsystem<UStableDiffusionSubsystem>();
	if (!subsystem)
		return;
	subsystem->InitModel();
}


//void UStableDiffusionViewportWidget::GenerateImage(const FString& prompt, FIntPoint size, float InputStrength, int32 Iterations, int32 Seed)
//{
//	//auto Bridge = UStableDiffusionBridge::Get();
//	auto subsystem = GEditor->GetEditorSubsystem<UStableDiffusionSubsystem>();
//	if (subsystem) {
//		subsystem->OnImageGenerationComplete.AddLambda([this, subsystem, prompt, size](const TArray<FColor>& PixelData) {
//			auto BrushTex = Cast<UTexture2D>(ViewportImage->Brush.GetResourceObject());
//			if (BrushTex) {
//				subsystem->ColorBufferToTexture(prompt, PixelData, size, BrushTex);
//				ViewportImage->SetBrushFromTexture(BrushTex);
//			}
//
//			//if (ViewportImage) {
//			//	auto ViewportTexture = ViewportImage->
//			//	subsystem->ColorBufferToTexture(prompt, PixelData, size, this->ViewportImage);
//			//	// TODO: Will this leak when replaced?
//			//	if (texture) {
//			//		DisplayedTexture = texture;
//			//		ViewportImage->SetBrushFromTexture(DisplayedTexture);
//			//	}
//			//}
//		});
//		subsystem->GenerateImage(prompt, size, InputStrength, Iterations, Seed);
//	}
//}

void UStableDiffusionViewportWidget::UpdateViewportImage(const FString& Prompt, FIntPoint Size, const TArray<FColor>& PixelData)
{
	auto BrushTex = Cast<UTexture2D>(ViewportImage->Brush.GetResourceObject());
	if (BrushTex) {
		auto subsystem = GEditor->GetEditorSubsystem<UStableDiffusionSubsystem>();
		if (subsystem) {
			subsystem->ColorBufferToTexture(Prompt, PixelData, Size, BrushTex);
			ViewportImage->SetBrushFromTexture(BrushTex);
		}
	}
}
