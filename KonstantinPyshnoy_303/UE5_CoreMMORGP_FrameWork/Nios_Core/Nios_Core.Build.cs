using System.IO;
// Some copyright should be here...

using UnrealBuildTool;

public class Nios_Core : ModuleRules
{
	public Nios_Core(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		
		PublicIncludePaths.AddRange(
            new string[] {
                Path.Combine(ModuleDirectory),
                Path.Combine(ModuleDirectory, "Core"),
                Path.Combine(ModuleDirectory, "Character"),
                Path.Combine(ModuleDirectory, "AbilitySystem"),
                Path.Combine(ModuleDirectory, "Components"),
                Path.Combine(ModuleDirectory, "Data"),
                Path.Combine(ModuleDirectory, "UI"),
                Path.Combine(ModuleDirectory, "Rules")
            }
            );
				
		
		PrivateIncludePaths.AddRange(
            new string[] {
                Path.Combine(ModuleDirectory)
            }
            );
			
		
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				// ... add other public dependencies that you statically link with here ...
			"CoreUObject", 
			"Engine", 
			"InputCore", 
			"EnhancedInput",
			"GameplayAbilities",
			"GameplayTags",
			"GameplayTasks",
			"Niagara",
			"OnlineSubsystem",
			"OnlineSubsystemSteam",
			"GameplayMessageRuntime",
    			"UMG",
    			"Slate",
    			"SlateCore",
			"Niagara",
			"DeveloperSettings",
			"MissionObjectives" 			}
			);
			
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"Engine",
				"Slate",
				"SlateCore",
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
