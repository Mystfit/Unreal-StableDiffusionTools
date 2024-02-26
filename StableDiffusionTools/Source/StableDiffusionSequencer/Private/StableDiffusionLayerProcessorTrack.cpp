// Copyright Epic Games, Inc. All Rights Reserved.

#include "StableDiffusionLayerProcessorTrack.h"

#include "Evaluation/MovieSceneEvaluationField.h"
#include "EntitySystem/BuiltInComponentTypes.h"
#include "EntitySystem/MovieSceneEntitySystemLinker.h"
#include "MovieSceneTracksComponentTypes.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(StableDiffusionLayerProcessorTrack)

#define LOCTEXT_NAMESPACE "StableDiffusionLayerProcessorTrack"

/*
namespace UE
{
	namespace MovieScene
	{
		static TUniquePtr<FLayerProcessorComponentTypes> GLayerProcessorComponentTypes;

		FLayerProcessorComponentTypes* FLayerProcessorComponentTypes::Get()
		{
			if (!GLayerProcessorComponentTypes.IsValid())
			{
				GLayerProcessorComponentTypes.Reset(new FLayerProcessorComponentTypes);
			}
			return GLayerProcessorComponentTypes.Get();
		}

		FLayerProcessorComponentTypes::FLayerProcessorComponentTypes()
		{
			using namespace UE::MovieScene;

			FComponentRegistry* ComponentRegistry = UMovieSceneEntitySystemLinker::GetComponents();

			//ComponentRegistry->NewComponentType(&LayerData, TEXT("Layer Data"));

			//ComponentRegistry->Factories.DuplicateChildComponent(LayerData);
		}

	} // namespace MovieScene
} // namespace UE
*/

UStableDiffusionLayerProcessorTrack::UStableDiffusionLayerProcessorTrack(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	BuiltInTreePopulationMode = ETreePopulationMode::Blended;

	SupportedBlendTypes.Add(EMovieSceneBlendType::Absolute);

#if WITH_EDITORONLY_DATA
	TrackTint = FColor(210,30,150,65);
#endif
}

UMovieSceneSection* UStableDiffusionLayerProcessorTrack::CreateNewSection()
{
	UMovieSceneSection* NewSection = NewObject<UStableDiffusionLayerProcessorSection>(this, NAME_None, RF_Transactional);
	NewSection->SetRange(TRange<FFrameNumber>::All());
	NewSection->SetBlendType(EMovieSceneBlendType::Absolute);
	return NewSection;
}

void UStableDiffusionLayerProcessorTrack::ImportEntityImpl(UMovieSceneEntitySystemLinker* EntityLinker, const FEntityImportParams& Params, FImportedEntity* OutImportedEntity)
{
	// These tracks don't define any entities for themselves
	checkf(false, TEXT("This track should never have created entities for itself - this assertion indicates an error in the entity-component field"));
}

void UStableDiffusionLayerProcessorTrack::ExtendEntityImpl(UMovieSceneParameterSection* Section, UMovieSceneEntitySystemLinker* EntityLinker, const UE::MovieScene::FEntityImportParams& Params, UE::MovieScene::FImportedEntity* OutImportedEntity)
{
	using namespace UE::MovieScene;

	FBuiltInComponentTypes* BuiltInComponents = FBuiltInComponentTypes::Get();
	//FMovieSceneTracksComponentTypes* TracksComponents = FMovieSceneTracksComponentTypes::Get();

	// Parameters are always absolute blends for the time being
	//TComponentTypeID<FLayerProcessorContext> LayerDataTypeID;
	//FLayerProcessorComponentTypes* LayerProcessorComponents = FLayerProcessorComponentTypes::Get();

	/*
	OutImportedEntity->AddBuilder(
		FEntityBuilder()
		.Add(LayerProcessorComponents->LayerData, LayerData)
		.AddTag(BuiltInComponents->Tags.AbsoluteBlend)
		.AddTag(BuiltInComponents->Tags.Master)
	);*/
}

bool UStableDiffusionLayerProcessorTrack::PopulateEvaluationFieldImpl(const TRange<FFrameNumber>& EffectiveRange, const FMovieSceneEvaluationFieldEntityMetaData& InMetaData, FMovieSceneEntityComponentFieldBuilder* OutFieldBuilder)
{
	const FMovieSceneTrackEvaluationField& LocalEvaluationField = GetEvaluationField();

	// Define entities for every entry in our evaluation field
	for (const FMovieSceneTrackEvaluationFieldEntry& Entry : LocalEvaluationField.Entries)
	{
		UMovieSceneParameterSection* ParameterSection = Cast<UMovieSceneParameterSection>(Entry.Section);
		if (!ParameterSection || IsRowEvalDisabled(ParameterSection->GetRowIndex()))
		{
			continue;
		}

		TRange<FFrameNumber> SectionEffectiveRange = TRange<FFrameNumber>::Intersection(EffectiveRange, Entry.Range);
		if (!SectionEffectiveRange.IsEmpty())
		{
			FMovieSceneEvaluationFieldEntityMetaData SectionMetaData = InMetaData;
			SectionMetaData.Flags = Entry.Flags;

			ParameterSection->ExternalPopulateEvaluationField(SectionEffectiveRange, SectionMetaData, OutFieldBuilder);
		}
	}

	return true;
}

bool UStableDiffusionLayerProcessorTrack::SupportsType(TSubclassOf<UMovieSceneSection> SectionClass) const
{
	return SectionClass == UStableDiffusionLayerProcessorSection::StaticClass();
}

#if WITH_EDITORONLY_DATA
FText UStableDiffusionLayerProcessorTrack::GetDefaultDisplayName() const
{
	return LOCTEXT("DefaultTrackName", "Stable Diffusion Layer Processor");
}
#endif

#undef LOCTEXT_NAMESPACE

