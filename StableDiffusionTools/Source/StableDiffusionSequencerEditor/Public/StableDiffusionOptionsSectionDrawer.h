#pragma once

#include "CoreMinimal.h"
#include "StableDiffusionOptionsSection.h"
#include "ISequencerSection.h"

class STABLEDIFFUSIONSEQUENCEREDITOR_API FStableDiffusionOptionsSectionDrawer : public  ISequencerSection
{
public:
	FStableDiffusionOptionsSectionDrawer(UMovieSceneSection& InSection, TWeakPtr<ISequencer> InSequencer);

	virtual int32 OnPaintSection(FSequencerSectionPainter& InPainter) const override;
	virtual void CustomizePropertiesDetailsView(TSharedRef<IDetailsView> DetailsView, const FSequencerSectionPropertyDetailsViewCustomizationParams& InParams) const override;
	virtual UMovieSceneSection* GetSectionObject() override;
	virtual FText GetSectionTitle() const override;
	virtual float GetSectionHeight() const override;

private:
	UStableDiffusionOptionsSection* OptionsSection;
};


class FMovieSceneStableDiffusionOptionsDetailCustomization : public IPropertyTypeCustomization
{
public:
	FMovieSceneStableDiffusionOptionsDetailCustomization(const FSequencerSectionPropertyDetailsViewCustomizationParams& InParams)
		: Params(InParams)
	{
	}

	// IDetailCustomization interface
	virtual void CustomizeHeader(TSharedRef<IPropertyHandle> PropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& CustomizationUtils) override
	{
	}

	virtual void CustomizeChildren(TSharedRef<IPropertyHandle> PropertyHandle, IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& CustomizationUtils) override;

	bool ShouldFilterAsset(const FAssetData& AssetData)
	{
		// Since the `SObjectPropertyEntryBox` doesn't support passing some `Filter` properties for the asset picker, 
		// we just combine the tag value filtering we want (i.e. checking the skeleton compatibility) along with the
		// other filtering we already get from the track editor's filter callback.
		/*FSkeletalAnimationTrackEditor& TrackEditor = static_cast<FSkeletalAnimationTrackEditor&>(Params.TrackEditor);
		if (TrackEditor.ShouldFilterAsset(AssetData))
		{
			return true;
		}

		return !(Skeleton && Skeleton->IsCompatibleSkeletonByAssetData(AssetData));*/
	}

private:
	FSequencerSectionPropertyDetailsViewCustomizationParams Params;
};
