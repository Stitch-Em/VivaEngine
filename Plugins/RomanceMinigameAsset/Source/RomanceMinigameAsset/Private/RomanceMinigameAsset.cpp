// Copyright Epic Games, Inc. All Rights Reserved.

#include "RomanceMinigameAsset.h"
#include "RomanceMinigameAction.h"
#include "IAssetTools.h"
#include "AssetToolsModule.h"
#include "Modules/ModuleManager.h"
#include "Styling/SlateStyleRegistry.h"
#include "Styling/SlateStyle.h"

#define LOCTEXT_NAMESPACE "FRomanceMinigameAssetModule"

#define IMAGE_BRUSH(RelativePath, ...) FSlateImageBrush( Style->RootToContentDir( RelativePath, TEXT(".png") ), __VA_ARGS__ )

void FRomanceMinigameAssetModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
	IAssetTools& AssetTools = IAssetTools::Get();
	EAssetTypeCategories::Type assetType = AssetTools.RegisterAdvancedAssetCategory(FName(TEXT("VivaEngine")), FText::FromString("VivaEngine"));
	TSharedPtr<RomanceMinigameAction> Action = MakeShareable(new RomanceMinigameAction(assetType));
	AssetTools.RegisterAssetTypeActions(Action.ToSharedRef());

	StyleWasImplemented = SetupStyle();

	if (StyleWasImplemented)
	{
		FSlateStyleRegistry::RegisterSlateStyle(*Style);
	}

	
}

void FRomanceMinigameAssetModule::ShutdownModule()
{
	if (StyleWasImplemented)
	{
		FSlateStyleRegistry::UnRegisterSlateStyle(Style->GetStyleSetName());
	}
}

bool FRomanceMinigameAssetModule::SetupStyle()
{
	FString RootDirectory;

	/**
	* Instead of using the IPluginManager to get the content directory in a second editor module its often easier to
	* to just find the directory in the most obvious places and check if its present. This is by no way a good solution in my opinion but it works.
	*/
	auto ProjectPluginDir = FPaths::ConvertRelativePathToFull(FPaths::Combine(FPaths::ProjectPluginsDir() + "RomanceMinigameAsset"));
	auto EnginePluginDir = FPaths::ConvertRelativePathToFull(FPaths::Combine(FPaths::EnginePluginsDir() + TEXT("Marketplace/RomanceMinigameAsset")));

	// Is it in the project plugin folder?
	if (FPaths::DirectoryExists(ProjectPluginDir))
	{
		RootDirectory = ProjectPluginDir;
	}
	// Is it in the engines marketplace folder ?
	else if (FPaths::DirectoryExists(EnginePluginDir))
	{
		RootDirectory = EnginePluginDir;
	}
	// No valid directory found, therefor no style will be registered or implemented
	else return false;

	// Create a new style set
	Style = MakeShareable(new FSlateStyleSet("YourPluginModuleStyle"));

	// Set the content Root directory of our plugin in order to access the images
	Style->SetContentRoot(RootDirectory);

	// Common sizes for icons and thumbnails
	const FVector2D Icon256x256(256.f, 256.f);

	// Set the Images of the properties to be equal of our new images, finding the property names can be a bit tricky however.
	Style->Set("ClassThumbnail.RomanceMinigame", new IMAGE_BRUSH(TEXT("Resources/HeartIconColor"), Icon256x256));

	// Continue to add styles below...

	return true;
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FRomanceMinigameAssetModule, RomanceMinigameAsset)