// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "StableDiffusionLayerProcessorOptionsTrack.h"
#include "EntitySystem/IMovieSceneEntityProvider.h"
#include "LayerProcessorBase.h"
#include "StableDiffusionLayerProcessorTrack.generated.h"



namespace UE
{
	namespace MovieScene
	{

		/*struct FLayerProcessorComponentData
		{
			FLayerProcessorComponentData() : Section(nullptr) {}
			FLayerProcessorComponentData(const UMovieSceneParameterSection* InSection) : Section(InSection) {}

			const UMovieSceneParameterSection* Section = nullptr;
		};*/

		struct STABLEDIFFUSIONSEQUENCER_API FLayerProcessorComponentTypes
		{
		public:
			static FLayerProcessorComponentTypes* Get();

			TComponentTypeID<FLayerData> LayerData;

		private:
			FLayerProcessorComponentTypes();
		};

	} // namespace MovieScene
} // namespace UE


/**
 * Handles manipulation of layer processors in a movie scene.
 */
UCLASS()
class STABLEDIFFUSIONSEQUENCER_API UStableDiffusionLayerProcessorTrack
	: public UStableDiffusionLayerProcessorOptionsTrack
	, public IMovieSceneEntityProvider
	, public IMovieSceneParameterSectionExtender
{
public:

	GENERATED_BODY()

	/** The layer to manipulate */
	UPROPERTY(EditAnywhere, Category = "Layer")
	TObjectPtr<ULayerProcessorBase> LayerProcessor;
	
	UPROPERTY(EditAnywhere, Category = "Layer")
	FString Role;

	UPROPERTY(EditAnywhere, Category = "Layer")
	TEnumAsByte<ELayerImageType> LayerType;

	UStableDiffusionLayerProcessorTrack(const FObjectInitializer& ObjectInitializer);

	virtual bool SupportsType(TSubclassOf<UMovieSceneSection> SectionClass) const override;
	virtual UMovieSceneSection* CreateNewSection() override;

	/*~ IMovieSceneEntityProvider */
	virtual void ImportEntityImpl(UMovieSceneEntitySystemLinker* EntityLinker, const FEntityImportParams& Params, FImportedEntity* OutImportedEntity) override;
	virtual bool PopulateEvaluationFieldImpl(const TRange<FFrameNumber>& EffectiveRange, const FMovieSceneEvaluationFieldEntityMetaData& InMetaData, FMovieSceneEntityComponentFieldBuilder* OutFieldBuilder) override;

	/*~ IMovieSceneParameterSectionExtender */
	virtual void ExtendEntityImpl(UMovieSceneEntitySystemLinker* EntityLinker, const UE::MovieScene::FEntityImportParams& Params, UE::MovieScene::FImportedEntity* OutImportedEntity) override;

#if WITH_EDITORONLY_DATA
	virtual FText GetDefaultDisplayName() const override;
#endif
};
