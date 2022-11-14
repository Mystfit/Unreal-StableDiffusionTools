#pragma once

#include "CoreMinimal.h"
#include "Editor/Blutility/Classes/EditorUtilityWidget.h"
#include "Engine/Texture2D.h"
#include "Components/Image.h"
#include "EditorUtilityWidgetBlueprint.h"
#include "Widgets/Docking/SDockTab.h"
#include "StableDiffusionViewportWidget.generated.h"

/**
 *
 */
UCLASS(BlueprintType)
class STABLEDIFFUSIONTOOLSEDITOR_API UStableDiffusionViewportWidget : public UEditorUtilityWidget
{
	GENERATED_BODY()
public:
	static UEditorUtilityWidget* CreateViewportWidget(TSharedPtr<SDockTab> DockTab, TSubclassOf<UEditorUtilityWidget> NewWidgetClass);

	void ChangeTabWorld(UWorld* World, EMapChangeType MapChangeType);

	UFUNCTION(BlueprintCallable, Category = "StableDiffusion|Subsystem")
	UStableDiffusionSubsystem* GetStableDiffusionSubsystem();

	UPROPERTY(BlueprintReadOnly, Category = "StableDiffusion|UI", meta = (BindWidget))
	UImage* ViewportImage;

private:
	TSharedPtr<SDockTab> OwningDockTab;
	TSubclassOf<UEditorUtilityWidget> WidgetClass;
};
