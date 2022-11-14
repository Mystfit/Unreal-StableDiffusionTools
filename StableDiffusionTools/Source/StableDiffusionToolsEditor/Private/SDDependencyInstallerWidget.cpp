// Fill out your copyright notice in the Description page of Project Settings.

#include "SDDependencyInstallerWidget.h"
#include "LevelEditor.h"

UEditorUtilityWidget* USDDependencyInstallerWidget::CreateViewportWidget(TSharedPtr<SDockTab> DockTab, TSubclassOf<UEditorUtilityWidget> NewWidgetClass)
{
	UWorld* World = GEditor->GetEditorWorldContext().World();
	check(World);


	USDDependencyInstallerWidget* CreatedUMGWidget = CreateWidget<USDDependencyInstallerWidget>(GEditor->GetEditorWorldContext().World(), NewWidgetClass);

	if (CreatedUMGWidget)
	{
		// Editor Utility is flagged as transient to prevent from dirty the World it's created in when a property added to the Utility Widget is changed
		CreatedUMGWidget->SetFlags(RF_Transient);

		CreatedUMGWidget->OwningDockTab = DockTab;
		CreatedUMGWidget->WidgetClass = NewWidgetClass;
		DockTab->SetContent(CreatedUMGWidget->TakeWidget());

		FLevelEditorModule& LevelEditor = FModuleManager::LoadModuleChecked<FLevelEditorModule>("LevelEditor");
		LevelEditor.OnMapChanged().AddUObject(CreatedUMGWidget, &USDDependencyInstallerWidget::ChangeTabWorld);
	}

	return CreatedUMGWidget;
}

void USDDependencyInstallerWidget::ChangeTabWorld(UWorld* World, EMapChangeType MapChangeType) {

	if (MapChangeType == EMapChangeType::TearDownWorld)
	{
		// We need to Delete the UMG widget if we are tearing down the World it was built with.
		if (this && World == this->GetWorld())
		{
			OwningDockTab->SetContent(SNullWidget::NullWidget);
			this->Rename(nullptr, GetTransientPackage());
		}
	}
	else if (MapChangeType != EMapChangeType::SaveMap) {
		//UEditorUtilityWidget* ReplacementUMGWidget = CreateWidget<UEditorUtilityWidget>(GEditor->GetEditorWorldContext().World(), WidgetClass);
		//DockTab->SetContent(ReplacementUMGWidget->TakeWidget());
		CreateViewportWidget(this->OwningDockTab, this->WidgetClass);
	}
}



UStableDiffusionSubsystem* USDDependencyInstallerWidget::GetStableDiffusionSubsystem() {
	UStableDiffusionSubsystem* subsystem = nullptr;
	subsystem = GEditor->GetEditorSubsystem<UStableDiffusionSubsystem>();
	return subsystem;
}
