using System.IO;
using UnrealBuildTool;

public class BanetteGenerator : ModuleRules
{
	public BanetteGenerator(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;


		// if (Target.WindowsPlatform.Compiler.IsMSVC())
		if (Target.Platform == UnrealTargetPlatform.Win64)
		{
			PublicSystemLibraries.Add("kernel32.lib");
			PublicSystemLibraries.Add("advapi32.lib");
			PublicSystemLibraries.Add("bcrypt.lib");
			PublicSystemLibraries.Add("ntdll.lib");
			PublicSystemLibraries.Add("userenv.lib");
			PublicSystemLibraries.Add("ws2_32.lib");
			PublicSystemLibraries.Add("msvcrt.lib");
			PublicSystemLibraries.Add("Shlwapi.lib");
		}

		var CargoTarget = GetCargoTargetTriple();
		var CargoProfile = GetCargoProfile();
		var LibFileName = GetLibFileName();

		var LibPath = Path.Combine(ModuleDirectory, "generator", "target", CargoTarget, CargoProfile, LibFileName);
		if (Target.Platform == UnrealTargetPlatform.Win64)
		{
			// The plugin's PreBuildSteps generate this library after UBT evaluates module rules.
			PublicPreBuildLibraries.Add(LibPath);
		}
		else
		{
			PublicAdditionalLibraries.Add(LibPath);
		}


		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
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

	private string GetCargoTargetTriple()
	{
		return Target.Platform == UnrealTargetPlatform.Win64
			? "x86_64-pc-windows-msvc"
			: Target.Platform == UnrealTargetPlatform.Linux
				? "x86_64-unknown-linux-gnu"
				: throw new BuildException($"Unsupported Unreal platform for Rust cargo build: {Target.Platform}");
	}

	private string GetCargoProfile()
	{
		return Target.Configuration == UnrealTargetConfiguration.DebugGame ? "debug" : "release";
	}

	private string GetLibFileName()
	{
		if (Target.Platform == UnrealTargetPlatform.Win64)
		{
			if (Target.WindowsPlatform.Compiler.IsMSVC())
			{
				return "generator.lib";
			}

			if (Target.WindowsPlatform.Compiler.IsClang())
			{
				return "generator.lib";
			}
		}

		if (Target.Platform == UnrealTargetPlatform.Linux)
		{
			return "generator.a"; // Linux uses .a for static libraries
		}

		throw new BuildException($"Unsupported Unreal platform for Rust cargo build: {Target.Platform}");
	}
}