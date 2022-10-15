#include "StableDiffusionOptionsTrackEditor.h"
#include "StableDiffusionOptionsSectionDrawer.h"
#include "StableDiffusionOptionsTrack.h"
#include "StableDiffusionOptionsSection.h"
#include "SequencerUtilities.h"

#define LOCTEXT_NAMESPACE "SequencerAwesomness"


FStableDiffusionOptionsTrackEditor::FStableDiffusionOptionsTrackEditor(TSharedRef<ISequencer>InSequencer) : FMovieSceneTrackEditor(InSequencer) {}

TSharedRef<ISequencerTrackEditor> FStableDiffusionOptionsTrackEditor::CreateTrackEditor(TSharedRef<ISequencer> InSequencer)
{
	return MakeShareable(new FStableDiffusionOptionsTrackEditor(InSequencer));
}

//Please take a note that on Button click we are creating our Track and its section.
void FStableDiffusionOptionsTrackEditor::BuildAddTrackMenu(FMenuBuilder& MenuBuilder)
{
	MenuBuilder.AddMenuEntry(
		LOCTEXT("AddOptionsTrack", "Stable Diffusion Generation track"), //Label
		LOCTEXT("AddOptionsTrackTooltip", "Add a Stable Diffusion Generation track that will control the generation of SD frames over time"), //Tooltip
		FSlateIcon(FEditorStyle::GetStyleSetName(), "ClassIcon.TextRenderComponent"),
		FUIAction(FExecuteAction::CreateRaw(this, &FStableDiffusionOptionsTrackEditor::OnAddTrack)
		));
}

bool FStableDiffusionOptionsTrackEditor::SupportsType(TSubclassOf<UMovieSceneTrack> Type) const
{
	return UStableDiffusionOptionsTrack::StaticClass() == Type;
}


bool FStableDiffusionOptionsTrackEditor::SupportsSequence(UMovieSceneSequence* InSequence) const
{
	if (!IsValid(InSequence))
	{
		return false;
	}


	static UClass* LevelSequencerClass = FindObject<UClass>(ANY_PACKAGE, TEXT("LevelSequence"), true);
	bool bValidClasses = true;


	bValidClasses &= (LevelSequencerClass != nullptr);
	bValidClasses &= InSequence->GetClass()->IsChildOf(LevelSequencerClass);


	return bValidClasses;
}

void FStableDiffusionOptionsTrackEditor::OnAddTrack()
{
	UMovieScene* FocusedMovieScene = GetFocusedMovieScene();
	if (IsValid(FocusedMovieScene))
	{
		if (FocusedMovieScene->IsReadOnly())
		{
			return;
		}

		const FScopedTransaction Transaction(LOCTEXT("FStableDiffusionOptionsTrackEditor_Transaction", "Add Stable Diffusion options track"));
		FocusedMovieScene->Modify();

		UStableDiffusionOptionsTrack* NewTrack = FocusedMovieScene->AddMasterTrack<UStableDiffusionOptionsTrack>();
		checkf(NewTrack != nullptr, TEXT("Failed to create new Stable DIffusion options track."));

		UStableDiffusionOptionsSection* NewSection = AddNewOptionsSection(FocusedMovieScene, NewTrack);
		if (GetSequencer().IsValid())
		{
			GetSequencer()->OnAddTrack(NewTrack, FGuid());
		}
	}
}

UStableDiffusionOptionsSection* FStableDiffusionOptionsTrackEditor::AddNewOptionsSection(UMovieScene* MovieScene, UMovieSceneTrack* OptionsTrack)
{
	const FScopedTransaction Transaction(LOCTEXT("StableDiffusionOptionsSection_Transaction", "Add Stable Diffusion Options Section"));

	OptionsTrack->Modify();

	UStableDiffusionOptionsSection* Section = CastChecked<UStableDiffusionOptionsSection>(OptionsTrack->CreateNewSection());

	TRange<FFrameNumber> SectionRange = MovieScene->GetPlaybackRange();
	Section->SetRange(SectionRange);

	/*int32 RowIndex = -1;
	for (const UMovieSceneSection* TrackSection : OptionsTrack->GetAllSections())
	{
		RowIndex = FMath::Max(RowIndex, TrackSection->GetRowIndex());
	}
	Section->SetRowIndex(RowIndex + 1);*/

	//OptionsTrack->AddSection(*Section);

	return Section;
}

TSharedRef<ISequencerSection> FStableDiffusionOptionsTrackEditor::MakeSectionInterface(UMovieSceneSection& SectionObject, UMovieSceneTrack& Track, FGuid ObjectBinding)
{
	return MakeShareable(new FStableDiffusionOptionsSectionDrawer(SectionObject, GetSequencer()));
}


TSharedPtr<SWidget> FStableDiffusionOptionsTrackEditor::BuildOutlinerEditWidget(const FGuid& ObjectBinding, UMovieSceneTrack* Track, const FBuildEditWidgetParams& Params)
{
	// Create a container edit box
	//return FSequencerUtilities::MakeAddButton(
	//	LOCTEXT("AddStableDiffsionPromptTrack", "Prompt"),
	//	FOnGetContent::CreateSP(this, &FStableDiffusionOptionsTrackEditor::BuildAddPromptMenu, Track),
	//	Params.NodeIsHovered, GetSequencer());
	return nullptr;
}


TSharedRef<SWidget> FStableDiffusionOptionsTrackEditor::BuildAddOptionMenu(UMovieSceneTrack* OptionsTrack)
{
	FMenuBuilder MenuBuilder(true, nullptr);

	MenuBuilder.AddMenuEntry(
		LOCTEXT("AddGenerationOutput", "Generation Options"),
		LOCTEXT("AddGenerationOutputToolTip", "Add a section to control the generation of Stable Diffusion images over time"),
		FSlateIcon(),
		FUIAction(FExecuteAction::CreateSP(
			this, &FStableDiffusionOptionsTrackEditor::OnAddNewSection, OptionsTrack)));
	return MenuBuilder.MakeWidget();
}

void FStableDiffusionOptionsTrackEditor::OnAddNewSection(UMovieSceneTrack* OptionsTrack)
{
	UMovieScene* FocusedMovieScene = GetFocusedMovieScene();

	if (FocusedMovieScene == nullptr)
	{
		return;
	}

	UStableDiffusionOptionsSection* NewSection = AddNewOptionsSection(FocusedMovieScene, OptionsTrack);

	GetSequencer()->NotifyMovieSceneDataChanged(EMovieSceneDataChangeType::MovieSceneStructureItemAdded);
	GetSequencer()->EmptySelection();
	GetSequencer()->SelectSection(NewSection);
	GetSequencer()->ThrobSectionSelection();
}



#undef LOCTEXT_NAMESPACE