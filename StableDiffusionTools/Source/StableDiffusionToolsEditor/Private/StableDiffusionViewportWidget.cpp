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
