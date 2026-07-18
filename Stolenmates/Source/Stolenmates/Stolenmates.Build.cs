// Fill out your copyright notice in the Description page of Project Settings.

using UnrealBuildTool;

public class Stolenmates : ModuleRules
{
    public Stolenmates(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "InputCore",
            "OnlineSubsystem",
            "OnlineSubsystemUtils"
        });

        PrivateDependencyModuleNames.AddRange(new string[]
        {
        });

        DynamicallyLoadedModuleNames.Add("OnlineSubsystemSteam");

        if (Target.Platform == UnrealTargetPlatform.Win64 || Target.Platform == UnrealTargetPlatform.Win32)
        {
            AddEngineThirdPartyPrivateStaticDependencies(Target, "Steamworks");

            PublicAdditionalLibraries.Add("Xinput.lib");
        }
    }
}