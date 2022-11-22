// Copyright Epic Games, Inc. All Rights Reserved.

#include "StableDiffusionToolsCommands.h"

#define LOCTEXT_NAMESPACE "FStableDiffusionToolsModule"

void FStableDiffusionToolsCommands::RegisterCommands()
{
	UI_COMMAND(OpenPluginWindow, "Stable Diffusion Tools", "Bring up the Stable Diffusion Tools panel", EUserInterfaceActionType::Button, FInputChord());
	UI_COMMAND(OpenDependenciesWindow, "Stable Diffusion Tools Dependencies", "Bring up Stable Diffusion Tools dependencies panel", EUserInterfaceActionType::Button, FInputChord());
}

#undef LOCTEXT_NAMESPACE
