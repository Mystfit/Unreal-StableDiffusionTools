// Copyright Epic Games, Inc. All Rights Reserved.

#include "StableDiffusionLayerProcessorTrackEditor.h"

#include "Algo/Sort.h"
#include "AssetRegistry/ARFilter.h"
#include "AssetRegistry/AssetData.h"
#include "Containers/UnrealString.h"
#include "ContentBrowserDelegates.h"
#include "ContentBrowserModule.h"
#include "Delegates/Delegate.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/Commands/UIAction.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Framework/SlateDelegates.h"
#include "HAL/Platform.h"
#include "HAL/PlatformCrt.h"
#include "IContentBrowserSingleton.h"
#include "ISequencer.h"
#include "ISequencerTrackEditor.h"
#include "Internationalization/Internationalization.h"
#include "Internationalization/Text.h"
#include "Materials/MaterialParameterCollection.h"
#include "Misc/AssertionMacros.h"
#include "Misc/Attribute.h"
#include "Misc/FrameNumber.h"
#include "Misc/Guid.h"
#include "Modules/ModuleManager.h"
#include "MovieScene.h"
#include "MovieSceneSection.h"
#include "MovieSceneSequence.h"
#include "MovieSceneTrack.h"
#include "ScopedTransaction.h"
#include "Sections/MovieSceneParameterSection.h"
#include "StableDiffusionParameterSection.h"
#include "SequencerUtilities.h"
#include "Styling/SlateIconFinder.h"
#include "Templates/Casts.h"
#include "Textures/SlateIcon.h"
#include "Types/SlateStructs.h"
#include "UObject/Class.h"
#include "UObject/NameTypes.h"
#include "UObject/Object.h"
#include "UObject/ObjectPtr.h"
#include "UObject/TopLevelAssetPath.h"
#include "UObject/UnrealNames.h"
#include "UObject/WeakObjectPtr.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/Layout/SBox.h"
#include "StableDiffusionLayerProcessorTrack.h"

class ISequencerSection;
class SWidget;
struct FSlateBrush;

#define LOCTEXT_NAMESPACE "StableDiffusionLayerProcessorTrackEditor"


FStableDiffusionLayerProcessorTrackEditor::FStableDiffusionLayerProcessorTrackEditor(TSharedRef<ISequencer> InSequencer)
	: FMovieSceneTrackEditor(InSequencer)
{
}

TSharedRef<ISequencerTrackEditor> FStableDiffusionLayerProcessorTrackEditor::CreateTrackEditor(TSharedRef<ISequencer> OwningSequencer)
{
	return MakeShared<FStableDiffusionLayerProcessorTrackEditor>(OwningSequencer);
}

TSharedRef<ISequencerSection> FStableDiffusionLayerProcessorTrackEditor::MakeSectionInterface(UMovieSceneSection& SectionObject, UMovieSceneTrack& Track, FGuid ObjectBinding)
{
	UMovieSceneParameterSection* ParameterSection = Cast<UMovieSceneParameterSection>(&SectionObject);
	checkf(ParameterSection != nullptr, TEXT("Unsupported section type."));
	return MakeShareable(new FStableDiffusionParameterSection(*ParameterSection));
}

TSharedRef<SWidget> CreateAssetPicker(FOnAssetSelected OnAssetSelected, FOnAssetEnterPressed OnAssetEnterPressed, TWeakPtr<ISequencer> InSequencer)
{
	UMovieSceneSequence* Sequence = InSequencer.IsValid() ? InSequencer.Pin()->GetFocusedMovieSceneSequence() : nullptr;

	FAssetPickerConfig AssetPickerConfig;
	{
		AssetPickerConfig.OnAssetSelected = OnAssetSelected;
		AssetPickerConfig.OnAssetEnterPressed = OnAssetEnterPressed;
		AssetPickerConfig.bAllowNullSelection = false;
		AssetPickerConfig.InitialAssetViewType = EAssetViewType::List;
		AssetPickerConfig.Filter.bRecursiveClasses = true;
		AssetPickerConfig.Filter.ClassPaths.Add(ULayerProcessorBase::StaticClass()->GetClassPathName());
		AssetPickerConfig.SaveSettingsName = TEXT("SequencerAssetPicker");
		AssetPickerConfig.AdditionalReferencingAssets.Add(FAssetData(Sequence));
	}

	FContentBrowserModule& ContentBrowserModule = FModuleManager::Get().LoadModuleChecked<FContentBrowserModule>(TEXT("ContentBrowser"));

	return SNew(SBox)
		.WidthOverride(300.0f)
		.HeightOverride(300.f)
		[
			ContentBrowserModule.Get().CreateAssetPicker(AssetPickerConfig)
		];
}

void FStableDiffusionLayerProcessorTrackEditor::BuildTrackContextMenu(FMenuBuilder& MenuBuilder, UMovieSceneTrack* Track)
{
	UStableDiffusionLayerProcessorTrack* MPCTrack = Cast<UStableDiffusionLayerProcessorTrack>(Track);

	auto AssignAsset = [MPCTrack](const FAssetData& InAssetData)
	{
		ULayerProcessorBase* Processor = Cast<ULayerProcessorBase>(InAssetData.GetAsset());

		if (Processor)
		{
			FScopedTransaction Transaction(LOCTEXT("SetAssetTransaction", "Assign Layer Processor"));
			MPCTrack->Modify();
			MPCTrack->SetDisplayName(FText::FromString(Processor->GetName()));
			MPCTrack->LayerProcessor = Processor;
			MPCTrack->LayerType = Processor->DefaultLayerType;
			MPCTrack->Role = "image";
		}

		FSlateApplication::Get().DismissAllMenus();
	};

	auto AssignAssetEnterPressed = [AssignAsset](const TArray<FAssetData>& InAssetData)
	{
		if (InAssetData.Num() > 0)
		{
			AssignAsset(InAssetData[0].GetAsset());
		}
	};

	auto SubMenuCallback = [this, AssignAsset, AssignAssetEnterPressed](FMenuBuilder& SubMenuBuilder)
	{
		SubMenuBuilder.AddWidget(CreateAssetPicker(FOnAssetSelected::CreateLambda(AssignAsset), FOnAssetEnterPressed::CreateLambda(AssignAssetEnterPressed), GetSequencer()), FText::GetEmpty(), true);
	};

	MenuBuilder.AddSubMenu(
		LOCTEXT("SetAsset", "Set Asset"),
		LOCTEXT("SetAsset_ToolTip", "Sets the layer processor that this track animates."),
		FNewMenuDelegate::CreateLambda(SubMenuCallback)
	);
}


void FStableDiffusionLayerProcessorTrackEditor::BuildAddTrackMenu(FMenuBuilder& MenuBuilder)
{
	auto SubMenuCallback = [this](FMenuBuilder& SubMenuBuilder)
	{
		SubMenuBuilder.AddWidget(CreateAssetPicker(FOnAssetSelected::CreateRaw(this, &FStableDiffusionLayerProcessorTrackEditor::AddTrackToSequence), FOnAssetEnterPressed::CreateRaw(this, &FStableDiffusionLayerProcessorTrackEditor::AddTrackToSequenceEnterPressed), GetSequencer()), FText::GetEmpty(), true);
	};

	MenuBuilder.AddSubMenu(
		LOCTEXT("AddLayerProcessorTrack", "Stable Diffusion Layer Processor Track"),
		LOCTEXT("AddlayerProcessorTrackToolTip", "Adds a new track that controls parameters within a Layer Processor."),
		FNewMenuDelegate::CreateLambda(SubMenuCallback),
		false,
		FSlateIconFinder::FindIconForClass(ULayerProcessorBase::StaticClass())
	);
}

void FStableDiffusionLayerProcessorTrackEditor::AddTrackToSequence(const FAssetData& InAssetData)
{
	FSlateApplication::Get().DismissAllMenus();

	ULayerProcessorBase* Processor = Cast<ULayerProcessorBase>(InAssetData.GetAsset());
	UMovieScene* MovieScene = GetFocusedMovieScene();
	if (!Processor || !MovieScene)
	{
		return;
	}

	if (MovieScene->IsReadOnly())
	{
		return;
	}

	// Attempt to find an existing MPC track that animates this object
	//for (UMovieSceneTrack* Track : MovieScene->GetMasterTracks())
	//{
	//	if (auto* MPCTrack = Cast<UStableDiffusionLayerProcessorTrack>(Track))
	//	{
	//		if (MPCTrack->MPC == MPC)
	//		{
	//			return;
	//		}
	//	}
	//}

	const FScopedTransaction Transaction(LOCTEXT("AddTrackDescription", "Add Layer Processor Track"));

	MovieScene->Modify();
	UStableDiffusionLayerProcessorTrack* Track = MovieScene->AddMasterTrack<UStableDiffusionLayerProcessorTrack>();
	check(Track);

	UMovieSceneSection* NewSection = Track->CreateNewSection();
	check(NewSection);

	Track->AddSection(*NewSection);
	Track->LayerProcessor = Processor;
	Track->LayerType = Processor->DefaultLayerType;
	Track->Role = "image";
	Track->SetDisplayName(FText::FromString(Processor->GetName()));

	if (GetSequencer().IsValid())
	{
		GetSequencer()->OnAddTrack(Track, FGuid());
	}
}

void FStableDiffusionLayerProcessorTrackEditor::AddTrackToSequenceEnterPressed(const TArray<FAssetData>& InAssetData)
{
	if (InAssetData.Num() > 0)
	{
		AddTrackToSequence(InAssetData[0].GetAsset());
	}
}

bool FStableDiffusionLayerProcessorTrackEditor::SupportsType(TSubclassOf<UMovieSceneTrack> Type) const
{
	return Type == UStableDiffusionLayerProcessorTrack::StaticClass();
}

bool FStableDiffusionLayerProcessorTrackEditor::SupportsSequence(UMovieSceneSequence* InSequence) const
{
	/*ETrackSupport TrackSupported = InSequence ? InSequence->IsTrackSupported(UStableDiffusionLayerProcessorTrack::StaticClass()) : ETrackSupport::NotSupported;
	return TrackSupported == ETrackSupport::Supported;*/
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

const FSlateBrush* FStableDiffusionLayerProcessorTrackEditor::GetIconBrush() const
{
	return FSlateIconFinder::FindIconForClass(ULayerProcessorBase::StaticClass()).GetIcon();
}

TSharedPtr<SWidget> FStableDiffusionLayerProcessorTrackEditor::BuildOutlinerEditWidget(const FGuid& ObjectBinding, UMovieSceneTrack* Track, const FBuildEditWidgetParams& Params)
{
	UStableDiffusionLayerProcessorTrack* MPCTrack = Cast<UStableDiffusionLayerProcessorTrack>(Track);
	FOnGetContent MenuContent = FOnGetContent::CreateSP(this, &FStableDiffusionLayerProcessorTrackEditor::OnGetAddParameterMenuContent, MPCTrack);

	return FSequencerUtilities::MakeAddButton(LOCTEXT("AddParameterButton", "Parameter"), MenuContent, Params.NodeIsHovered, GetSequencer());
}

TSharedRef<SWidget> FStableDiffusionLayerProcessorTrackEditor::OnGetAddParameterMenuContent(UStableDiffusionLayerProcessorTrack* LayerProcessorTrack)
{
	FMenuBuilder MenuBuilder(true, nullptr);

	/*MenuBuilder.BeginSection(NAME_None, LOCTEXT("ScalarParametersHeading", "Scalar"));
	{
		TArray<FCollectionScalarParameter> ScalarParameters = MPCTrack->MPC->ScalarParameters;
		Algo::SortBy(ScalarParameters, &FCollectionParameterBase::ParameterName, FNameLexicalLess());

		for (const FCollectionScalarParameter& Scalar : ScalarParameters)
		{
			MenuBuilder.AddMenuEntry(
				FText::FromName(Scalar.ParameterName),
				FText(),
				FSlateIcon(),
				FExecuteAction::CreateSP(this, &FStableDiffusionLayerProcessorTrackEditor::AddScalarParameter, MPCTrack, Scalar)
				);
		}
	}
	MenuBuilder.EndSection();*/

	MenuBuilder.BeginSection(NAME_None, LOCTEXT("ScalarParametersHeading", "Scalar"));
	{
		//TArray<FCollectionVectorParameter> VectorParameters = MPCTrack->MPC->VectorParameters;
		//Algo::SortBy(VectorParameters, &FCollectionParameterBase::ParameterName, FNameLexicalLess());

		if (IsValid(LayerProcessorTrack->LayerProcessor)) {
			if(auto Options = LayerProcessorTrack->LayerProcessor->AllocateLayerOptions()){
				for (TFieldIterator<FProperty> PropertyIt(Options->GetClass()); PropertyIt; ++PropertyIt)
				{
					FProperty* Prop = *PropertyIt;
					if (Prop) {
						//FString Name;
						//Prop->GetName(Name);

						if (const FFloatProperty* FloatProperty = CastField<FFloatProperty>(Prop)) {
							FCollectionScalarParameter Param;
							Param.ParameterName = Prop->GetFName();
							Prop->GetValue_InContainer(Options, &Param.DefaultValue);

							MenuBuilder.AddMenuEntry(
								FText::FromName(Prop->GetFName()),
								FText(),
								FSlateIcon(),
								FExecuteAction::CreateSP(this, &FStableDiffusionLayerProcessorTrackEditor::AddScalarParameter, LayerProcessorTrack, Param)
							);
						}
					}
				}
			}
		}

		//for (const FCollectionVectorParameter& Vector : VectorParameters)
		//{
		//	
		//}
	}
	MenuBuilder.EndSection();

	return MenuBuilder.MakeWidget();
}


void FStableDiffusionLayerProcessorTrackEditor::AddScalarParameter(UStableDiffusionLayerProcessorTrack* Track, FCollectionScalarParameter Parameter)
{
	if (!Track->LayerProcessor)
	{
		return;
	}

	FFrameNumber KeyTime = GetTimeForKey();

	const FScopedTransaction Transaction(LOCTEXT("AddScalarParameter", "Add scalar parameter"));
	Track->Modify();
	Track->AddScalarParameterKey(Parameter.ParameterName, KeyTime, Parameter.DefaultValue);
	GetSequencer()->NotifyMovieSceneDataChanged(EMovieSceneDataChangeType::MovieSceneStructureItemAdded);
}


//void FStableDiffusionLayerProcessorTrackEditor::AddVectorParameter(UStableDiffusionLayerProcessorTrack* Track, FCollectionVectorParameter Parameter)
//{
//	if (!Track->LayerData.Processor)
//	{
//		return;
//	}
//
//	FFrameNumber KeyTime = GetTimeForKey();
//
//	const FScopedTransaction Transaction(LOCTEXT("AddVectorParameter", "Add vector parameter"));
//	Track->Modify();
//	Track->AddColorParameterKey(Parameter.ParameterName, KeyTime, Parameter.DefaultValue);
//	GetSequencer()->NotifyMovieSceneDataChanged(EMovieSceneDataChangeType::MovieSceneStructureItemAdded);
//}

bool FStableDiffusionLayerProcessorTrackEditor::HandleAssetAdded(UObject* Asset, const FGuid& TargetObjectGuid)
{
	ULayerProcessorBase* MPC = Cast<ULayerProcessorBase>(Asset);
	if (MPC)
	{
		AddTrackToSequence(FAssetData(MPC));
	}

	return MPC != nullptr;
}

#undef LOCTEXT_NAMESPACE
