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
	UFUNCTION(BlueprintCallable, Category = "StableDiffusion|Subsystem")
	UStableDiffusionSubsystem* GetStableDiffusionSubsystem();

	UPROPERTY(BlueprintReadOnly, Category = "StableDiffusion|UI", meta = (BindWidget))
	UImage* ViewportImage;
};
