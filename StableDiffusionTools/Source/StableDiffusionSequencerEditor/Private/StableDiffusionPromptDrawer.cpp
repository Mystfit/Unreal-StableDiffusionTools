#include "StableDiffusionPromptSectionDrawer.h"
#include "SequencerSectionPainter.h"

FStableDiffusionPromptSectionDrawer::FStableDiffusionPromptSectionDrawer(UMovieSceneSection& InSection, TWeakPtr<ISequencer> InSequencer)
{
	PromptSection = Cast<UStableDiffusionPromptMovieSceneSection>(&InSection);
}

int32 FStableDiffusionPromptSectionDrawer::OnPaintSection(FSequencerSectionPainter& InPainter) const
{
	return InPainter.PaintSectionBackground(InPainter.BlendColor(PromptSection->Prompt.Sentiment == EPromptSentiment::Positive ? FLinearColor(0.1, 0.4, 0.1) : FLinearColor(0.4, 0.1, 0.1)));
}

UMovieSceneSection* FStableDiffusionPromptSectionDrawer::GetSectionObject()
{
	return PromptSection;
}

FText FStableDiffusionPromptSectionDrawer::GetSectionTitle() const
{
	return FText::FromString(PromptSection->Prompt.Prompt);
}

float FStableDiffusionPromptSectionDrawer::GetSectionHeight() const
{
	return 20.0f;
}
