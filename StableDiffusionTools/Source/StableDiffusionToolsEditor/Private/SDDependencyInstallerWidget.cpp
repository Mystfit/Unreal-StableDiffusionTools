// Fill out your copyright notice in the Description page of Project Settings.


#include "SDDependencyInstallerWidget.h"


UStableDiffusionSubsystem* USDDependencyInstallerWidget::GetStableDiffusionSubsystem() {
	UStableDiffusionSubsystem* subsystem = nullptr;
	subsystem = GEditor->GetEditorSubsystem<UStableDiffusionSubsystem>();
	return subsystem;
}
