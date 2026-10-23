using UnrealBuildTool;

public class BanetteTransport : ModuleRules
{
	public BanetteTransport(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.Default;

		// Resolve module-qualified headers through Unreal's parent include paths.
		bLegacyParentIncludePaths = true;

		PublicDependencyModuleNames.AddRange(
			new[]
			{
				"Core",
				"CoreUObject",
				"HTTP",
				"UE5Coro",
				"Banette",
				"Json",
				"JsonUtilities"
			}
		);

		PrivateDependencyModuleNames.AddRange(
			new[]
			{
				"Engine"
			}
		);
	}
}