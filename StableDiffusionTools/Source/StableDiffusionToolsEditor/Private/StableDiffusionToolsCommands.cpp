// Copyright Epic Games, Inc. All Rights Reserved.

#include "StableDiffusionToolsCommands.h"

#define LOCTEXT_NAMESPACE "FStableDiffusionToolsModule"

void FStableDiffusionToolsCommands::RegisterCommands()
{
	UI_COMMAND(OpenPluginWindow, "StableDiffusionTools", "Bring up StableDiffusionTools window", EUserInterfaceActionType::Button, FInputChord());
}

#undef LOCTEXT_NAMESPACE
