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
class STABLEDIFFUSIONTOOLS_API UStableDiffusionViewportWidget : public UEditorUtilityWidget
{
	GENERATED_BODY()

public:
	/*virtual void NativeConstruct() override;

	virtual TSharedRef<SWidget> RebuildWidget() override;*/
	UFUNCTION(BlueprintCallable)
	UStableDiffusionSubsystem* GetStableDiffusionSubsystem();

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	UImage* ViewportImage;

	UFUNCTION(BlueprintCallable)
	void InstallDependencies();

	UFUNCTION(BlueprintCallable)
	void InitModel(FIntPoint size);

	//UFUNCTION(BlueprintCallable)
	//void GenerateImage(const FString& prompt, FIntPoint size, float InputStrength, int32 Iterations, int32 Seed);

	UFUNCTION(BlueprintCallable)
	void UpdateViewportImage(const FString& Prompt, FIntPoint Size, const TArray<FColor>& PixelData);

private:
	TObjectPtr<UTexture2D> DisplayedTexture;
};
