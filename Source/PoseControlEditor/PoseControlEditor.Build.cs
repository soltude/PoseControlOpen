// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class PoseControlEditor : ModuleRules
{
	public PoseControlEditor(ReadOnlyTargetRules Target) : base(Target)
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
				"Core", "PhysicsUtilities","ModelingComponentsEditorOnly", "Blutility", 
				// ... add other public dependencies that you statically link with here ...
				"Persona",
				"AssetTools"
			}
			);
			
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"AnimationCore",
                "CableComponent",
                "CoreUObject", 
                "InputCore", 
				"EditorFramework",
				"Engine",
                "EnhancedInput",
                "PhysicsCore",
                "PhysicsControl", 
                "ModelViewViewModel", 
                "PhysicsControlEditor",
                "PhysicsAssetEditor",
                "PoseControl",
                "Projects",
				"ToolMenus",
				"Slate",
				"SlateCore",
                "UMG",
                "GeometryCore",
				"UnrealEd", 
				"Blutility",
				"Boost",
				"ModelingComponents",
				"ModelingComponentsEditorOnly",
				"EditorScriptingUtilities",
				"AssetTools",

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
