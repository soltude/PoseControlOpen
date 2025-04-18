// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class PcCommon : ModuleRules
{
	public PcCommon(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		
		PublicIncludePaths.AddRange(
			new string[] {
				// ... add public include paths required here ...
			}
			);
				
		
		PrivateIncludePaths.AddRange(
			new string[] {
				// ... add other private include paths required here ...
			}
			);
			
		
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"AssetTools",
                "SlateCore",
                "UMG",
                "PhysicsControl",
                "PhysicsUtilities",
                "Engine",
				"ModelViewViewModel", 
				// ... add other public dependencies that you statically link with here ...
			}
			);
			
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
                "CableComponent",
                "CoreUObject", 
                "InputCore", 
				"Engine",
                "EnhancedInput",
                "PhysicsCore",
                "PhysicsControl", 
                "ModelViewViewModel", 
				// ... add private dependencies that you statically link with here ...	
			}
			);
		
		
		DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
				// ... add any modules that your module loads dynamically here ...
			}
			);
	}
}
