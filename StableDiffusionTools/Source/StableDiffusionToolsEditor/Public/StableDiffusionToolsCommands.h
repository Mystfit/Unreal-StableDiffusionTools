// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Framework/Commands/Commands.h"
#include "StableDiffusionToolsStyle.h"

class FStableDiffusionToolsCommands : public TCommands<FStableDiffusionToolsCommands>
{
public:

	FStableDiffusionToolsCommands()
		: TCommands<FStableDiffusionToolsCommands>(TEXT("StableDiffusionTools"), NSLOCTEXT("Contexts", "StableDiffusionTools", "StableDiffusionTools Plugin"), NAME_None, FStableDiffusionToolsStyle::GetStyleSetName())
	{
	}

	// TCommands<> interface
	virtual void RegisterCommands() override;

public:
	TSharedPtr< FUICommandInfo > OpenPluginWindow;
	TSharedPtr< FUICommandInfo > OpenDependenciesWindow;
	TSharedPtr< FUICommandInfo > OpenModelToolsWindow;
};