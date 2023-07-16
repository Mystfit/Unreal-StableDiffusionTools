#pragma once

#include "CoreMinimal.h"
#include "Editor/Blutility/Classes/EditorUtilityWidget.h"
#include "Engine/Texture2D.h"
#include "Components/Image.h"
#include "SAssetDropTarget.h"
#include "EditorUtilityWidgetBlueprint.h"
#include "Widgets/Docking/SDockTab.h"
#include "Components/NativeWidgetHost.h"
#include "StableDiffusionViewportWidget.generated.h"


/**
 *
 */
UCLASS(BlueprintType)
class STABLEDIFFUSIONTOOLSEDITOR_API UStableDiffusionViewportWidget : 
	public UEditorUtilityWidget,
	public TSharedFromThis<UStableDiffusionViewportWidget>
{
	GENERATED_BODY()
public:
	virtual void NativeConstruct() override;

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "StableDIffusion|UI")
	void UpdateViewportImage(UTexture* Texture, FIntPoint Size);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "StableDIffusion|UI")
	void UpdateFromDataAsset(UStableDiffusionImageResultAsset* Asset);

	UPROPERTY(BlueprintReadOnly, Category = "StableDiffusion|UI", meta = (BindWidget))
	UNativeWidgetHost* AssetDropTarget;

	UPROPERTY(BlueprintReadOnly, Category = "StableDiffusion|UI", meta = (BindWidget))
	UImage* ViewportImage;

	/* Pipeline stages */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Instanced, Category = "Stages")
	TArray<UImagePipelineStageAsset*> PipelineStages;
private:
};
