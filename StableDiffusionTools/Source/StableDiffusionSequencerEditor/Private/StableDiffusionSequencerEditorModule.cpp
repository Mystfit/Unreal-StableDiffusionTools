#include "StableDiffusionSequencerEditorModule.h"
#include "ISequencerModule.h"
#include "StableDiffusionPromptTrackEditor.h"
#include "StableDiffusionOptionsTrackEditor.h"
#include "StableDiffusionLayerProcessorTrackEditor.h"

void FStableDiffusionSequencerEditorModule::StartupModule() {
	ISequencerModule& SequencerModule = FModuleManager::LoadModuleChecked<ISequencerModule>("Sequencer");
	SequencerModule.RegisterTrackEditor(FOnCreateTrackEditor::CreateStatic(&FStableDiffusionPromptTrackEditor::CreateTrackEditor));
	SequencerModule.RegisterTrackEditor(FOnCreateTrackEditor::CreateStatic(&FStableDiffusionOptionsTrackEditor::CreateTrackEditor));
	SequencerModule.RegisterTrackEditor(FOnCreateTrackEditor::CreateStatic(&FStableDiffusionLayerProcessorTrackEditor::CreateTrackEditor));
}

void FStableDiffusionSequencerEditorModule::ShutdownModule() {

}

IMPLEMENT_MODULE(FStableDiffusionSequencerEditorModule, StableDiffusionSequencerEditor)
