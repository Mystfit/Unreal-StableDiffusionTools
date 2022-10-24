#pragma once

#include "CoreMinimal.h"
#include "Editor/Blutility/Classes/EditorUtilityWidget.h"
#include "Engine/Texture2D.h"
#include "Components/Image.h"
#include "StableDiffusionViewportWidget.generated.h"

/**
 *
 */
UCLASS(BlueprintType)
class STABLEDIFFUSIONTOOLSEDITOR_API UStableDiffusionViewportWidget : public UEditorUtilityWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable)
	UStableDiffusionSubsystem* GetStableDiffusionSubsystem();

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	UImage* ViewportImage;

private:
	TObjectPtr<UTexture2D> DisplayedTexture;
};
