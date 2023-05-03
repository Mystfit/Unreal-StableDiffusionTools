#include "StableDiffusionToolsSettings.h"
#include "StableDiffusionSubsystem.h"
#include "DetailLayoutBuilder.h"
#include "DetailCategoryBuilder.h"
#include "DetailWidgetRow.h"
#include "Widgets/Input/SButton.h"

TSubclassOf<UStableDiffusionBridge> UStableDiffusionToolsSettings::GetGeneratorType() const {
	return GeneratorType;
}

TMap<FName, FString> UStableDiffusionToolsSettings::GetGeneratorTokens() const {
	return GeneratorTokens;
}

void UStableDiffusionToolsSettings::AddGeneratorToken(const FName& Generator)
{
	if (!GeneratorTokens.Contains(Generator)) {
		GeneratorTokens.Add(Generator);
	}
}

TSharedRef<IDetailCustomization> FStableDiffusionToolsSettingsDetails::MakeInstance()
{
	return MakeShareable(new FStableDiffusionToolsSettingsDetails);
}

void FStableDiffusionToolsSettingsDetails::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
	TArray<TWeakObjectPtr<UObject>> ObjectsBeingCustomized;
	DetailBuilder.GetObjectsBeingCustomized(ObjectsBeingCustomized);
	check(ObjectsBeingCustomized.Num() == 1);
	TWeakObjectPtr<UStableDiffusionToolsSettings> Settings = Cast<UStableDiffusionToolsSettings>(ObjectsBeingCustomized[0].Get());

	IDetailCategoryBuilder& OptionsCategory = DetailBuilder.EditCategory("Options");
	OptionsCategory.AddCustomRow(FText::FromString("Open token website for selected generator"))
		.ValueContent()
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
				.Padding(5)
				.AutoWidth()
				[
					SNew(SButton)
					.Text(FText::FromString("Open token website for generator type"))
					.ToolTipText(FText::FromString("Open a browser window for your chosen generator that will allow you to obtain a valid token you can add to the 'Generator Tokens' map. You may need to log in or register account depending on the active generator."))
					.IsEnabled_Lambda([this, Settings]() {
						return(Settings->GetGeneratorType() != nullptr);
					})
					.OnClicked_Lambda([this, Settings]()
					{
						if (auto subsystem = GEditor->GetEditorSubsystem<UStableDiffusionSubsystem>()) {
							if (auto generator = subsystem->GeneratorBridge) {
								FPlatformProcess::LaunchURL(*generator->GetTokenWebsiteHint(), NULL, NULL);
							}
						}
						return(FReply::Handled());
					})
				]
			];
}
