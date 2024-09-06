// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

#include "Styling/SlateStyle.h"
#include "Styling/SlateStyleRegistry.h"

class FRomanceMinigameAssetModule : public IModuleInterface
{
public:

	// Style of the plugin
	TSharedPtr<FSlateStyleSet> Style;

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

private:

	// Was the Style implement correctly?
	bool StyleWasImplemented;

	// Applies the Icons for the plugins asset.
	bool SetupStyle();
};
