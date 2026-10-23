using UnrealBuildTool;

public class BanetteKit : ModuleRules
{
	public BanetteKit(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.Default;

		// Resolve module-qualified headers through Unreal's parent include paths.
		bLegacyParentIncludePaths = true;

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"UE5Coro",
				"Banette",
				"BanetteTransport",
				"Json",
				"JsonUtilities",
			}
		);

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"Engine",
				"Slate",
				"SlateCore"
			}
		);
	}
}