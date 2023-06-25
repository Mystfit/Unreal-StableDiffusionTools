// Copyright Epic Games, Inc. All Rights Reserved.

#include "StableDiffusionToolsCommands.h"

#define LOCTEXT_NAMESPACE "FStableDiffusionToolsModule"

void FStableDiffusionToolsCommands::RegisterCommands()
{
	UI_COMMAND(OpenPluginWindow, "Diffusion Tools UI", "Bring up the Stable Diffusion Tools panel", EUserInterfaceActionType::Button, FInputChord());
	UI_COMMAND(OpenDependenciesWindow, "Diffusion Tools Dependencies", "Bring up Stable Diffusion Tools dependencies panel", EUserInterfaceActionType::Button, FInputChord());
	UI_COMMAND(OpenModelToolsWindow, "Diffusion Tools Model browser", "Bring up Stable Diffusion Tools model browser panel", EUserInterfaceActionType::Button, FInputChord());
}

#undef LOCTEXT_NAMESPACE
