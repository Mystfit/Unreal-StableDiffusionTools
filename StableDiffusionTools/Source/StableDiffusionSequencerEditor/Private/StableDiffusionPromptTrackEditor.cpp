#include "StableDiffusionPromptTrackEditor.h"
#include "StableDiffusionPromptSectionDrawer.h"
#include "StableDiffusionOptionsTrack.h"
#include "StableDiffusionPromptMovieSceneTrack.h"
#include "StableDiffusionPromptMovieSceneSection.h"
#include "SequencerUtilities.h"

#define LOCTEXT_NAMESPACE "SequencerAwesomness"


FStableDiffusionPromptTrackEditor::FStableDiffusionPromptTrackEditor(TSharedRef<ISequencer>InSequencer) : FMovieSceneTrackEditor(InSequencer) {}

TSharedRef<ISequencerTrackEditor> FStableDiffusionPromptTrackEditor::CreateTrackEditor(TSharedRef<ISequencer> InSequencer)
{
	return MakeShareable(new FStableDiffusionPromptTrackEditor(InSequencer));
}

//Please take a note that on Button click we are creating our Track and its section.
void FStableDiffusionPromptTrackEditor::BuildAddTrackMenu(FMenuBuilder& MenuBuilder)
{
	MenuBuilder.AddMenuEntry(
		LOCTEXT("AddPromptTrack", "Stable Diffusion Prompt track"), //Label
		LOCTEXT("AddPromptTrackTooltip", "Add a Stable Diffusion prompt track that will control the text prompt sent to the image generator"), //Tooltip
		FSlateIcon(FAppStyle::GetAppStyleSetName(), "ClassIcon.TextRenderComponent"),
		FUIAction(FExecuteAction::CreateRaw(this, &FStableDiffusionPromptTrackEditor::OnAddTrack)
	));
}

bool FStableDiffusionPromptTrackEditor::SupportsType(TSubclassOf<UMovieSceneTrack> Type) const
{
	return UStableDiffusionPromptMovieSceneTrack::StaticClass() == Type;
}


bool FStableDiffusionPromptTrackEditor::SupportsSequence(UMovieSceneSequence* InSequence) const
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

void FStableDiffusionPromptTrackEditor::OnAddTrack()
{
	UMovieScene* FocusedMovieScene = GetFocusedMovieScene();
	if (IsValid(FocusedMovieScene))
	{
		if (FocusedMovieScene->IsReadOnly())
		{
			return;
		}

		const FScopedTransaction Transaction(LOCTEXT("FStableDiffusionPromptTrackEditor_Transaction", "Add Stable Diffusion prompt track"));
		FocusedMovieScene->Modify();

		UStableDiffusionPromptMovieSceneTrack* NewTrack = FocusedMovieScene->AddMasterTrack<UStableDiffusionPromptMovieSceneTrack>();
		checkf(NewTrack != nullptr, TEXT("Failed to create new Stable Diffusion prompt track."));

		UStableDiffusionPromptMovieSceneSection* NewSection = AddNewPromptSection(FocusedMovieScene, NewTrack);
		if (GetSequencer().IsValid())
		{
			GetSequencer()->OnAddTrack(NewTrack, FGuid());
		}
	}
}

UStableDiffusionPromptMovieSceneSection* FStableDiffusionPromptTrackEditor::AddNewPromptSection(UMovieScene* MovieScene, UMovieSceneTrack* PromptTrack)
{
	const FScopedTransaction Transaction(LOCTEXT("StableDiffusionPromptSection_Transaction", "Add Stable Diffusion prompt Section"));

	PromptTrack->Modify();

	UStableDiffusionPromptMovieSceneSection* Section = CastChecked<UStableDiffusionPromptMovieSceneSection>(PromptTrack->CreateNewSection());

	TRange<FFrameNumber> SectionRange = MovieScene->GetPlaybackRange();
	Section->SetRange(SectionRange);

	int32 RowIndex = -1;
	for (const UMovieSceneSection* TrackSection : PromptTrack->GetAllSections())
	{
		RowIndex = FMath::Max(RowIndex, TrackSection->GetRowIndex());
	}
	Section->SetRowIndex(RowIndex + 1);

	//PromptTrack->AddSection(*Section);

	return Section;
}

TSharedRef<ISequencerSection> FStableDiffusionPromptTrackEditor::MakeSectionInterface(UMovieSceneSection& SectionObject, UMovieSceneTrack& Track, FGuid ObjectBinding)
{
	return MakeShareable(new FStableDiffusionPromptSectionDrawer(SectionObject, GetSequencer()));
}


TSharedPtr<SWidget> FStableDiffusionPromptTrackEditor::BuildOutlinerEditWidget(const FGuid& ObjectBinding, UMovieSceneTrack* Track, const FBuildEditWidgetParams& Params)
{
	// Create a container edit box
	return FSequencerUtilities::MakeAddButton(
		LOCTEXT("AddStableDiffsionPromptTrack", "Prompt"),
		FOnGetContent::CreateSP(this, &FStableDiffusionPromptTrackEditor::BuildAddPromptMenu, Track),
		Params.NodeIsHovered, GetSequencer());
	//return nullptr;
}


TSharedRef<SWidget> FStableDiffusionPromptTrackEditor::BuildAddPromptMenu(UMovieSceneTrack* OptionsTrack)
{
	FMenuBuilder MenuBuilder(true, nullptr);

	MenuBuilder.AddMenuEntry(
		LOCTEXT("AddPromptSection", "Prompt"),
		LOCTEXT("AddPromptSectionToolTip", "Adds a section that will feed a new weighted prompt to the image generator"),
		FSlateIcon(),
		FUIAction(FExecuteAction::CreateSP(
			this, &FStableDiffusionPromptTrackEditor::OnAddNewSection, OptionsTrack)));
	return MenuBuilder.MakeWidget();
}

void FStableDiffusionPromptTrackEditor::OnAddNewSection(UMovieSceneTrack* OptionsTrack)
{
	UMovieScene* FocusedMovieScene = GetFocusedMovieScene();

	if (FocusedMovieScene == nullptr)
	{
		return;
	}

	UStableDiffusionPromptMovieSceneSection* NewSection = AddNewPromptSection(FocusedMovieScene, OptionsTrack);

	GetSequencer()->NotifyMovieSceneDataChanged(EMovieSceneDataChangeType::MovieSceneStructureItemAdded);
	GetSequencer()->EmptySelection();
	GetSequencer()->SelectSection(NewSection);
	GetSequencer()->ThrobSectionSelection();
}



#undef LOCTEXT_NAMESPACE