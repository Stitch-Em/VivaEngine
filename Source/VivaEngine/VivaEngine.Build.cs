// Fill out your copyright notice in the Description page of Project Settings.

using UnrealBuildTool;
using System.IO;

public class VivaEngine : ModuleRules
{
	public VivaEngine(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
	
		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "UMG" });

		PrivateDependencyModuleNames.AddRange(new string[] {  });

        // Get the directory path where the Discord files are located
        string DiscordFilesDirectory = Path.Combine(ModuleDirectory, "discord-files");
        PublicIncludePaths.Add(DiscordFilesDirectory);

        // Get the path to the lib file
        string BinariesDirectory = Path.Combine(ModuleDirectory, "../../Binaries", Target.Platform.ToString());
        string LibFilePath = Path.Combine(BinariesDirectory, "discord_game_sdk.dll.lib");
        PublicAdditionalLibraries.Add(LibFilePath);

        // Uncomment if you are using Slate UI
        // PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

        // Uncomment if you are using online features
        // PrivateDependencyModuleNames.Add("OnlineSubsystem");

        // To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
    }
}
