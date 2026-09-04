using UnrealBuildTool;

public class GSKitEditor : ModuleRules
{
    public GSKitEditor(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        CppStandard = CppStandardVersion.Cpp20;

        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "UnrealEd"
        });

        PublicIncludePaths.AddRange(new string[]
        {
            ModuleDirectory + "/Public"
        });
    }
}
