// Fill out your copyright notice in the Description page of Project Settings.

#include "ModioEditorUtilityFunctions.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "CoreGlobals.h"
#include "Editor.h"
#include "Modules/ModuleManager.h"
#include "Templates/UnrealTemplate.h"
#include "ModioEditorSettings.h"
#include "LevelEditor.h"

void UModioEditorUtilityFunctions::SelectAssetsInContentBrowser(const TArray<FString>& AssetPaths)
{
	TGuardValue<bool> UnattendedScriptGuard(GIsRunningUnattendedScript, true);

	if (GEditor)
	{
		TArray<FAssetData> Assets;
		for (const FString& AssetPath : AssetPaths)
		{
			FString FailureReason;
			FAssetRegistryModule& AssetRegistryModule =
				FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
			FAssetData Asset = AssetRegistryModule.Get().GetAssetByObjectPath(*AssetPath);
			if (Asset.IsValid())
			{
				Assets.Add(Asset);
			}
		}
		if (Assets.Num() > 0)
		{
			GEditor->SyncBrowserToObjects(Assets);
		}
	}
}

void UModioEditorUtilityFunctions::SetDisplayGettingStartedWidgetOnStartup(bool bNewValue) 
{
	if (UModioEditorSettings* EditorSettings = GetMutableDefault<UModioEditorSettings>())
	{
		EditorSettings->bShowGettingStartedOnStartup = bNewValue;
		EditorSettings->SaveConfig();
	}
}

void UModioEditorUtilityFunctions::OpenTutorialBrowser()
{
	FLevelEditorModule& LevelEditorModule = FModuleManager::GetModuleChecked<FLevelEditorModule>(TEXT("LevelEditor"));
	LevelEditorModule.GetLevelEditorTabManager()->TryInvokeTab(FTabId("TutorialsBrowser"));
}