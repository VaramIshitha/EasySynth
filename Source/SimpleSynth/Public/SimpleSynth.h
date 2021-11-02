// Copyright Ydrive 2021

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

#include "WidgetManager.h"

class FToolBarBuilder;
class FMenuBuilder;


// Declare SimpleSynth log category
DECLARE_LOG_CATEGORY_EXTERN(LogSimpleSynth, Log, All);


class FSimpleSynthModule : public IModuleInterface
{
public:
	/** IModuleInterface startup implementation */
	void StartupModule() override;

	/** IModuleInterface shutdown implementation */
	void ShutdownModule() override;

	/** This function will be bound to Command (by default it will bring up plugin window) */
	void PluginButtonClicked();

private:
	/** Adds the plugin to editor menus */
	void RegisterMenus();

private:
	TSharedPtr<class FUICommandList> PluginCommands;

	/** Utility that manages editor tab UI */
	UWidgetManager WidgetManager;
};
