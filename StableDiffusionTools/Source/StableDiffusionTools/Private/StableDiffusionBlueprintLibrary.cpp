// Fill out your copyright notice in the Description page of Project Settings.

#include "StableDiffusionBlueprintLibrary.h"
#include "SLevelViewport.h"
#include "Dialogs/Dialogs.h"
#include "LevelEditor.h"

#define LOCTEXT_NAMESPACE "StableDiffusionBlueprintLibrary"

UStableDiffusionSubsystem* UStableDiffusionBlueprintLibrary::GetStableDiffusionSubsystem() {
	UStableDiffusionSubsystem* subsystem = nullptr;
	subsystem = GEditor->GetEditorSubsystem<UStableDiffusionSubsystem>();
	return subsystem;
}

UStableDiffusionToolsSettings* UStableDiffusionBlueprintLibrary::GetPluginOptions()
{
	auto result = GetMutableDefault<UStableDiffusionToolsSettings>();
	return result;
}

UDependencySettings* UStableDiffusionBlueprintLibrary::GetDependencyOptions()
{
	auto result = GetMutableDefault<UDependencySettings>();
	return result;
}

void UStableDiffusionBlueprintLibrary::RestartEditor()
{
	// Present the user with a warning that changing projects has to restart the editor
	FSuppressableWarningDialog::FSetupInfo Info(
		LOCTEXT("RestartEditorMsg", "The editor needs to restart to complete an operation."),
		LOCTEXT("RestartEditorTitle", "Restart editor"), "RestartEditorTitle_Warning");

	Info.ConfirmText = LOCTEXT("Yes", "Yes");
	Info.CancelText = LOCTEXT("No", "No");

	FSuppressableWarningDialog RestartDepsDlg(Info);
	bool bSwitch = true;
	if (RestartDepsDlg.ShowModal() == FSuppressableWarningDialog::Cancel)
	{
		bSwitch = false;
	}

	// If the user wants to continue with the restart set the pending project to swtich to and close the editor
	if (bSwitch)
	{
		FUnrealEdMisc::Get().RestartEditor(false);
	}
}

