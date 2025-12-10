// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class ar_integration : ModuleRules
{
	public ar_integration(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		CppStandard = CppStandardVersion.Cpp20; //C++20 not allowed with uwp

		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore" });

		PrivateDependencyModuleNames.AddRange(new string[] { "Research", "Grpc", "AugmentedReality", "ProceduralMeshComponent", "HeadMountedDisplay", "UXTools", "ProceduralMeshComponent", "XRBase" });

        PublicDefinitions.Add("WITH_POINTCLOUD");



        // Uncomment if you are using Slate UI
        // PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

        // Uncomment if you are using online features
        // PrivateDependencyModuleNames.Add("OnlineSubsystem");

        // To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
    }
}
