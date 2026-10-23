using UnrealBuildTool;

public class BanetteTest : ModuleRules
{
	public BanetteTest(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.Default;

		// Resolve module-qualified headers through Unreal's parent include paths.
		bLegacyParentIncludePaths = true;

		PublicDependencyModuleNames.AddRange(
			new[]
			{
				"Core",
				"Banette",
				"BanetteTransport",
				"BanetteKit",
				"UE5Coro",
				"Json",
				"JsonUtilities",
			}
		);

		PrivateDependencyModuleNames.AddRange(
			new[]
			{
				"CoreUObject",
				"Engine",
				"Slate",
				"SlateCore"
			}
		);
	}
}