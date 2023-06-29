// Copyright Epic Games, Inc. All Rights Reserved.

#include "StableDiffusionLayerProcessorOptionsTrack.h"
#include "MovieScene.h"
#include "MovieSceneCommonHelpers.h"
#include "Evaluation/MovieSceneEvaluationField.h"
#include "EntitySystem/MovieSceneEntityBuilder.h"
#include "EntitySystem/BuiltInComponentTypes.h"
#include "StableDiffusionLayerProcessorSection.h"
#include "MovieSceneTracksComponentTypes.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(StableDiffusionLayerProcessorOptionsTrack)

UStableDiffusionLayerProcessorOptionsTrack::UStableDiffusionLayerProcessorOptionsTrack(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITORONLY_DATA
	TrackTint = FColor(180,40,200,65);
#endif

	SupportedBlendTypes.Add(EMovieSceneBlendType::Absolute);
}


bool UStableDiffusionLayerProcessorOptionsTrack::SupportsType(TSubclassOf<UMovieSceneSection> SectionClass) const
{
	return SectionClass == UStableDiffusionLayerProcessorSection::StaticClass();
}

UMovieSceneSection* UStableDiffusionLayerProcessorOptionsTrack::CreateNewSection()
{
	return NewObject<UStableDiffusionLayerProcessorSection>(this, NAME_None, RF_Transactional);
}


void UStableDiffusionLayerProcessorOptionsTrack::RemoveAllAnimationData()
{
	Sections.Empty();
}


bool UStableDiffusionLayerProcessorOptionsTrack::HasSection(const UMovieSceneSection& Section) const
{
	return Sections.Contains(&Section);
}


void UStableDiffusionLayerProcessorOptionsTrack::AddSection(UMovieSceneSection& Section)
{
	Sections.Add(&Section);
}


void UStableDiffusionLayerProcessorOptionsTrack::RemoveSection(UMovieSceneSection& Section)
{
	Sections.Remove(&Section);
}

void UStableDiffusionLayerProcessorOptionsTrack::RemoveSectionAt(int32 SectionIndex)
{
	Sections.RemoveAt(SectionIndex);
}


bool UStableDiffusionLayerProcessorOptionsTrack::IsEmpty() const
{
	return Sections.Num() == 0;
}

bool UStableDiffusionLayerProcessorOptionsTrack::SupportsMultipleRows() const
{
	return true;
}

const TArray<UMovieSceneSection*>& UStableDiffusionLayerProcessorOptionsTrack::GetAllSections() const
{
	return Sections;
}


void UStableDiffusionLayerProcessorOptionsTrack::AddScalarParameterKey(FName ParameterName, FFrameNumber Time, float Value)
{
	UMovieSceneParameterSection* NearestSection = Cast<UMovieSceneParameterSection>(MovieSceneHelpers::FindNearestSectionAtTime(Sections, Time));
	if (NearestSection == nullptr)
	{
		NearestSection = Cast<UMovieSceneParameterSection>(CreateNewSection());

		UMovieScene* MovieScene = GetTypedOuter<UMovieScene>();
		check(MovieScene);

		NearestSection->SetRange(MovieScene->GetPlaybackRange());
		Sections.Add(NearestSection);
	}
	if (NearestSection->TryModify())
	{
		NearestSection->AddScalarParameterKey(ParameterName, Time, Value);
	}
}


void UStableDiffusionLayerProcessorOptionsTrack::AddColorParameterKey(FName ParameterName, FFrameNumber Time, FLinearColor Value)
{
	UMovieSceneParameterSection* NearestSection = Cast<UMovieSceneParameterSection>(MovieSceneHelpers::FindNearestSectionAtTime(Sections, Time));
	if (NearestSection == nullptr)
	{
		NearestSection = Cast<UMovieSceneParameterSection>(CreateNewSection());

		UMovieScene* MovieScene = GetTypedOuter<UMovieScene>();
		check(MovieScene);

		NearestSection->SetRange(MovieScene->GetPlaybackRange());

		Sections.Add(NearestSection);
	}
	if (NearestSection->TryModify())
	{
		NearestSection->AddColorParameterKey(ParameterName, Time, Value);
	}
}


//UMovieSceneComponentMaterialTrack::UMovieSceneComponentMaterialTrack(const FObjectInitializer& ObjectInitializer)
//	: Super(ObjectInitializer)
//{
//	BuiltInTreePopulationMode = ETreePopulationMode::Blended;
//}
//
//void UMovieSceneComponentMaterialTrack::AddSection(UMovieSceneSection& Section)
//{
//	// Materials are always blendable now
//	Section.SetBlendType(EMovieSceneBlendType::Absolute);
//	Super::AddSection(Section);
//}
//
//void UMovieSceneComponentMaterialTrack::ImportEntityImpl(UMovieSceneEntitySystemLinker* EntityLinker, const FEntityImportParams& Params, FImportedEntity* OutImportedEntity)
//{
//	// These tracks don't define any entities for themselves
//	checkf(false, TEXT("This track should never have created entities for itself - this assertion indicates an error in the entity-component field"));
//}
//
//void UMovieSceneComponentMaterialTrack::ExtendEntityImpl(UMovieSceneEntitySystemLinker* EntityLinker, const UE::MovieScene::FEntityImportParams& Params, UE::MovieScene::FImportedEntity* OutImportedEntity)
//{
//	using namespace UE::MovieScene;
//
//	FBuiltInComponentTypes* BuiltInComponents = FBuiltInComponentTypes::Get();
//	FMovieSceneTracksComponentTypes* TracksComponents = FMovieSceneTracksComponentTypes::Get();
//
//	// Material parameters are always absolute blends for the time being
//	OutImportedEntity->AddBuilder(
//		FEntityBuilder()
//		.Add(TracksComponents->ComponentMaterialIndex, MaterialIndex)
//		.AddTag(BuiltInComponents->Tags.AbsoluteBlend)
//	);
//}
//
//bool UMovieSceneComponentMaterialTrack::PopulateEvaluationFieldImpl(const TRange<FFrameNumber>& EffectiveRange, const FMovieSceneEvaluationFieldEntityMetaData& InMetaData, FMovieSceneEntityComponentFieldBuilder* OutFieldBuilder)
//{
//	const FMovieSceneTrackEvaluationField& LocalEvaluationField = GetEvaluationField();
//
//	// Define entities for every entry in our evaluation field
//	for (const FMovieSceneTrackEvaluationFieldEntry& Entry : LocalEvaluationField.Entries)
//	{
//		UMovieSceneParameterSection* ParameterSection = Cast<UMovieSceneParameterSection>(Entry.Section);
//		if (!ParameterSection || IsRowEvalDisabled(ParameterSection->GetRowIndex()))
//		{
//			continue;
//		}
//
//		TRange<FFrameNumber> SectionEffectiveRange = TRange<FFrameNumber>::Intersection(EffectiveRange, Entry.Range);
//		if (!SectionEffectiveRange.IsEmpty())
//		{
//			FMovieSceneEvaluationFieldEntityMetaData SectionMetaData = InMetaData;
//			SectionMetaData.Flags = Entry.Flags;
//
//			ParameterSection->ExternalPopulateEvaluationField(SectionEffectiveRange, SectionMetaData, OutFieldBuilder);
//		}
//	}
//
//	return true;
//}
//
//#if WITH_EDITORONLY_DATA
//FText UMovieSceneComponentMaterialTrack::GetDefaultDisplayName() const
//{
//	return FText::FromString(FString::Printf(TEXT("Material Element %i"), MaterialIndex));
//}
//#endif

