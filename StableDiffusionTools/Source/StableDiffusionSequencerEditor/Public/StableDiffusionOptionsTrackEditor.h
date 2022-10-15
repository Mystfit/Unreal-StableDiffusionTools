#pragma once

#include "CoreMinimal.h"
#include "MovieSceneTrackEditor.h"
#include "StableDiffusionOptionsSection.h"

class STABLEDIFFUSIONSEQUENCEREDITOR_API FStableDiffusionOptionsTrackEditor : public FMovieSceneTrackEditor
{
public:
	FStableDiffusionOptionsTrackEditor(TSharedRef<ISequencer>InSequencer);
	static TSharedRef<ISequencerTrackEditor> CreateTrackEditor(TSharedRef<ISequencer> InSequencer);

	//Add our track to the add track menu
	virtual void BuildAddTrackMenu(FMenuBuilder& MenuBuilder) override;

	//Draw the track entry itself (as this is an SWidget, it can be customized and additional button or labels can be added
	virtual TSharedPtr<SWidget> BuildOutlinerEditWidget(const FGuid& ObjectBinding, UMovieSceneTrack* Track, const FBuildEditWidgetParams& Params);
	TSharedRef<SWidget> BuildAddOptionMenu(UMovieSceneTrack* OptionsTrack);

	virtual TSharedRef<ISequencerSection> MakeSectionInterface(UMovieSceneSection& SectionObject, UMovieSceneTrack& Track, FGuid ObjectBinding) override;
	virtual bool SupportsType(TSubclassOf<UMovieSceneTrack> Type) const override;
	virtual bool SupportsSequence(UMovieSceneSequence* InSequence) const override;

	void OnAddTrack();

	//static FReply AddNewTrack(UMovieSceneTrack* track, UMovieScene* focusedMovieScene);
	UStableDiffusionOptionsSection* AddNewOptionsSection(UMovieScene* MovieScene, UMovieSceneTrack* OptionsTrack);
	void OnAddNewSection(UMovieSceneTrack* OptionsTrack);
};
