#pragma once

#include "CoreMinimal.h"
#include "StableDiffusionPromptMovieSceneSection.h"
#include "ISequencerSection.h"

class STABLEDIFFUSIONSEQUENCEREDITOR_API FStableDiffusionPromptSectionDrawer : public ISequencerSection
{
public:
	FStableDiffusionPromptSectionDrawer(UMovieSceneSection& InSection, TWeakPtr<ISequencer> InSequencer);

	virtual int32 OnPaintSection(FSequencerSectionPainter& InPainter) const override;
	virtual UMovieSceneSection* GetSectionObject() override;
	virtual FText GetSectionTitle() const override;
	virtual float GetSectionHeight() const override;

private:
	UStableDiffusionPromptMovieSceneSection* PromptSection;
};
