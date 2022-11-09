#include "StableDiffusionOptionsSectionDrawer.h"
#include "SequencerSectionPainter.h"
#include "IDetailPropertyRow.h"
#include "IDetailChildrenBuilder.h"

FStableDiffusionOptionsSectionDrawer::FStableDiffusionOptionsSectionDrawer(UMovieSceneSection& InSection, TWeakPtr<ISequencer> InSequencer)
{
	OptionsSection = Cast<UStableDiffusionOptionsSection>(&InSection);
}

int32 FStableDiffusionOptionsSectionDrawer::OnPaintSection(FSequencerSectionPainter& InPainter) const
{
	return InPainter.PaintSectionBackground();
}

void FStableDiffusionOptionsSectionDrawer::CustomizePropertiesDetailsView(TSharedRef<IDetailsView> DetailsView, const FSequencerSectionPropertyDetailsViewCustomizationParams& InParams) const
{
	DetailsView->RegisterInstancedCustomPropertyTypeLayout(
		TEXT("MovieSceneStableDiffusionOptionsParams"),
		FOnGetPropertyTypeCustomizationInstance::CreateLambda([=]() { return MakeShared<FMovieSceneStableDiffusionOptionsDetailCustomization>(InParams); }));
}

UMovieSceneSection* FStableDiffusionOptionsSectionDrawer::GetSectionObject()
{
	return OptionsSection;
}

FText FStableDiffusionOptionsSectionDrawer::GetSectionTitle() const
{
	return FText::FromString(FString(TEXT("Stable Diffusion Option Section")));
}

float FStableDiffusionOptionsSectionDrawer::GetSectionHeight() const
{
	return 40.0f;
}

void FMovieSceneStableDiffusionOptionsDetailCustomization::CustomizeChildren(TSharedRef<IPropertyHandle> PropertyHandle, IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& CustomizationUtils)
{
	//const FName AnimationPropertyName = GET_MEMBER_NAME_CHECKED(FMovieSceneSkeletalAnimationParams, Animation);
	//const FName MirrorDataTableName = GET_MEMBER_NAME_CHECKED(FMovieSceneSkeletalAnimationParams, MirrorDataTable);

	uint32 NumChildren;
	PropertyHandle->GetNumChildren(NumChildren);
	for (uint32 i = 0; i < NumChildren; ++i)
	{
		TSharedPtr<IPropertyHandle> ChildPropertyHandle = PropertyHandle->GetChildHandle(i);
		IDetailPropertyRow& ChildPropertyRow = ChildBuilder.AddProperty(ChildPropertyHandle.ToSharedRef());
		FName ChildPropertyName = ChildPropertyHandle->GetProperty()->GetFName();
		// Let most properties be whatever they want to be... we just want to customize the `Animation` and `MirrorDataTable` properties
		// by making it look like a normal asset reference property, but with some custom filtering.
		
		UE_LOG(LogTemp, Log, TEXT("Child property name: %s"), *ChildPropertyName.ToString())
		
		//if (ChildPropertyName == AnimationPropertyName || ChildPropertyName == MirrorDataTableName)
		//{
		//	FDetailWidgetRow& Row = ChildPropertyRow.CustomWidget();

		//	if (Params.ParentObjectBindingGuid.IsValid())
		//	{
		//		// Store the compatible skeleton's name, and create a property widget with a filter that will check
		//		// for animations that match that skeleton.
		//		Skeleton = AcquireSkeletonFromObjectGuid(Params.ParentObjectBindingGuid, Params.Sequencer);
		//		SkeletonName = FAssetData(Skeleton).GetExportTextName();

		//		TSharedPtr<IPropertyUtilities> PropertyUtilities = CustomizationUtils.GetPropertyUtilities();
		//		UClass* AllowedStaticClass = ChildPropertyName == AnimationPropertyName ? UAnimSequenceBase::StaticClass() : UMirrorDataTable::StaticClass();

		//		TSharedRef<SObjectPropertyEntryBox> ContentWidget = SNew(SObjectPropertyEntryBox)
		//			.PropertyHandle(ChildPropertyHandle)
		//			.AllowedClass(AllowedStaticClass)
		//			.DisplayThumbnail(true)
		//			.ThumbnailPool(PropertyUtilities.IsValid() ? PropertyUtilities->GetThumbnailPool() : nullptr)
		//			.OnShouldFilterAsset(FOnShouldFilterAsset::CreateRaw(this, &FMovieSceneSkeletalAnimationParamsDetailCustomization::ShouldFilterAsset));

		//		Row.NameContent()[ChildPropertyHandle->CreatePropertyNameWidget()];
		//		Row.ValueContent()[ContentWidget];

		//		float MinDesiredWidth, MaxDesiredWidth;
		//		ContentWidget->GetDesiredWidth(MinDesiredWidth, MaxDesiredWidth);
		//		Row.ValueContent().MinWidth = MinDesiredWidth;
		//		Row.ValueContent().MaxWidth = MaxDesiredWidth;

		//		// The content widget already contains a "reset to default" button, so we don't want the details view row
		//		// to make another one. We add this metadata on the property handle instance to suppress it.
		//		ChildPropertyHandle->SetInstanceMetaData(TEXT("NoResetToDefault"), TEXT("true"));
		//	}
		//}
	}
}
