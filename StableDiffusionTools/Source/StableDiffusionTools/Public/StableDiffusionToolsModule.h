#pragma once

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class FToolBarBuilder;
class FMenuBuilder;

class FStableDiffusionToolsModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
	void CreateSettingsSection();
	bool HandleSettingsSaved();

	void CreateDetailCustomizations();

private:
};
