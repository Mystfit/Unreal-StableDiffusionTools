// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "StableDiffusionLayerProcessorTrack.h"
#include "Containers/Array.h"
#include "MovieSceneTrackEditor.h"
#include "Templates/SharedPointer.h"
#include "Templates/SubclassOf.h"

class FMenuBuilder;
class ISequencer;
class ISequencerSection;
class ISequencerTrackEditor;
class SWidget;
class UMovieSceneSection;
class UMovieSceneSequence;
class UMovieSceneTrack;
class UObject;
struct FAssetData;
struct FBuildEditWidgetParams;
struct FCollectionScalarParameter;
struct FCollectionVectorParameter;
struct FGuid;
struct FSlateBrush;

/**
 * Track editor for layer processors
 */
class STABLEDIFFUSIONSEQUENCEREDITOR_API FStableDiffusionLayerProcessorTrackEditor : public FMovieSceneTrackEditor
{
public:

	/** Constructor. */
	FStableDiffusionLayerProcessorTrackEditor(TSharedRef<ISequencer> InSequencer);

	/** Factory function */
	static TSharedRef<ISequencerTrackEditor> CreateTrackEditor(TSharedRef<ISequencer> OwningSequencer);

public:

	//~ ISequencerTrackEditor interface
	virtual TSharedPtr<SWidget> BuildOutlinerEditWidget(const FGuid& ObjectBinding, UMovieSceneTrack* Track, const FBuildEditWidgetParams& Params) override;
	virtual TSharedRef<ISequencerSection> MakeSectionInterface(UMovieSceneSection& SectionObject, UMovieSceneTrack& Track, FGuid ObjectBinding) override;
	virtual bool SupportsType(TSubclassOf<UMovieSceneTrack> Type) const override;
	virtual bool SupportsSequence(UMovieSceneSequence* InSequence) const override;
	virtual void BuildAddTrackMenu(FMenuBuilder& MenuBuilder) override;
	virtual const FSlateBrush* GetIconBrush() const override;
	virtual bool HandleAssetAdded(UObject* Asset, const FGuid& TargetObjectGuid) override;
	virtual void BuildTrackContextMenu(FMenuBuilder& MenuBuilder, UMovieSceneTrack* Track) override;

private:

	/** Provides the contents of the add parameter menu. */
	TSharedRef<SWidget> OnGetAddParameterMenuContent(UStableDiffusionLayerProcessorTrack* LayerProcessorTrack);

	void AddScalarParameter(UStableDiffusionLayerProcessorTrack* Track, FCollectionScalarParameter Parameter);
	//void AddVectorParameter(UStableDiffusionLayerProcessorTrack* Track, FCollectionVectorParameter Parameter);

	void AddTrackToSequence(const FAssetData& InAssetData);
	void AddTrackToSequenceEnterPressed(const TArray<FAssetData>& InAssetData);
};
