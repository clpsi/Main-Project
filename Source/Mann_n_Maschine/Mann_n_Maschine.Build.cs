// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class Mann_n_Maschine : ModuleRules
{
	public Mann_n_Maschine(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
	
		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput",
			"ModelingComponents", "GeometryFramework", "GeometryCore", "GeometryScriptingCore", "DynamicMesh",
			"RealtimeMeshComponent", "RealtimeMeshExamples", "RealtimeMeshEditor", "RealtimeMeshTests", 
			"AssetRegistry" });

		PrivateDependencyModuleNames.AddRange(new string[] {  });

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });
		
		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
