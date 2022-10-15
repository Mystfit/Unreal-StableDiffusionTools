#include "StableDiffusionOptionsSectionDrawer.h"
#include "SequencerSectionPainter.h"

FStableDiffusionOptionsSectionDrawer::FStableDiffusionOptionsSectionDrawer(UMovieSceneSection& InSection, TWeakPtr<ISequencer> InSequencer)
{
	OptionsSection = Cast<UStableDiffusionOptionsSection>(&InSection);
}

int32 FStableDiffusionOptionsSectionDrawer::OnPaintSection(FSequencerSectionPainter& InPainter) const
{
	return InPainter.PaintSectionBackground();
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
