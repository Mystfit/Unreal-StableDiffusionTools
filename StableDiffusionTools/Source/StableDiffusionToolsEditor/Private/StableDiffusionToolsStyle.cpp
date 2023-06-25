// Copyright Epic Games, Inc. All Rights Reserved.

#include "StableDiffusionToolsStyle.h"
#include "Styling/SlateStyleRegistry.h"
#include "Framework/Application/SlateApplication.h"
#include "Slate/SlateGameResources.h"
#include "Interfaces/IPluginManager.h"
#include "Styling/SlateStyleMacros.h"

#define RootToContentDir Style->RootToContentDir

TSharedPtr<FSlateStyleSet> FStableDiffusionToolsStyle::StyleInstance = nullptr;

void FStableDiffusionToolsStyle::Initialize()
{
	if (!StyleInstance.IsValid())
	{
		StyleInstance = Create();
		FSlateStyleRegistry::RegisterSlateStyle(*StyleInstance);
	}
}

void FStableDiffusionToolsStyle::Shutdown()
{
	FSlateStyleRegistry::UnRegisterSlateStyle(*StyleInstance);
	ensure(StyleInstance.IsUnique());
	StyleInstance.Reset();
}

FName FStableDiffusionToolsStyle::GetStyleSetName()
{
	static FName StyleSetName(TEXT("StableDiffusionToolsStyle"));
	return StyleSetName;
}

const FVector2D Icon16x16(16.0f, 16.0f);
const FVector2D Icon20x20(20.0f, 20.0f);

TSharedRef< FSlateStyleSet > FStableDiffusionToolsStyle::Create()
{
	TSharedRef< FSlateStyleSet > Style = MakeShareable(new FSlateStyleSet("StableDiffusionToolsStyle"));
	Style->SetContentRoot(IPluginManager::Get().FindPlugin("StableDiffusionTools")->GetBaseDir() / TEXT("Resources"));
	Style->Set("StableDiffusionTools.OpenPluginWindow", new IMAGE_BRUSH_SVG(TEXT("ai-brain"), Icon20x20));
	Style->Set("StableDiffusionTools.OpenDependenciesWindow", new IMAGE_BRUSH_SVG(TEXT("download"), Icon20x20));
	Style->Set("StableDiffusionTools.OpenModelToolsWindow", new IMAGE_BRUSH_SVG(TEXT("globe"), Icon20x20));

	return Style;
}

void FStableDiffusionToolsStyle::ReloadTextures()
{
	if (FSlateApplication::IsInitialized())
	{
		FSlateApplication::Get().GetRenderer()->ReloadTextureResources();
	}
}

const ISlateStyle& FStableDiffusionToolsStyle::Get()
{
	return *StyleInstance;
}
