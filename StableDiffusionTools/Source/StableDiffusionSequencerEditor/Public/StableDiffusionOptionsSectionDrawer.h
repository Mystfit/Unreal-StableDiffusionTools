#pragma once

#include "CoreMinimal.h"
#include "StableDiffusionOptionsSection.h"
#include "ISequencerSection.h"

class STABLEDIFFUSIONSEQUENCEREDITOR_API FStableDiffusionOptionsSectionDrawer : public  ISequencerSection
{
public:
	FStableDiffusionOptionsSectionDrawer(UMovieSceneSection& InSection, TWeakPtr<ISequencer> InSequencer);

	virtual int32 OnPaintSection(FSequencerSectionPainter& InPainter) const override;
	virtual UMovieSceneSection* GetSectionObject() override;
	virtual FText GetSectionTitle() const override;
	virtual float GetSectionHeight() const override;

private:
	UStableDiffusionOptionsSection* OptionsSection;
};
