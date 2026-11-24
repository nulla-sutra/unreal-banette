using UnrealBuildTool;

public class Banette : ModuleRules
{
	public Banette(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		// Resolve module-qualified headers through Unreal's parent include paths.
		bLegacyParentIncludePaths = true;

		PublicDependencyModuleNames.AddRange(
			new[]
			{
				"Core",
				"CoreUObject",
				"UE5Coro",
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