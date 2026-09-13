// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class GASP : ModuleRules
{
	public GASP(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		CppCompileWarningSettings.NonInlinedGenCppWarningLevel = WarningLevel.Warning;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core", "CoreUObject", "Engine",
			"NetCore", "Mover", "NetworkPrediction",
			"GameplayTags", "GameplayTasks",
			"AIModule",
			"PoseSearch", "RigVM", "ControlRig",
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"EngineSettings",
			"MotionTrajectory", "Chooser", "BlendStack", "AnimationWarpingRuntime", "AnimGraphRuntime", "MotionWarping",
			"PhysicsCore", "PhysicsControl", "Niagara", "Chaos",
		});

		if (Target.Type == TargetRules.TargetType.Editor)
			PrivateDependencyModuleNames.AddRange(new string[]
			{
				"GameplayDebugger",
			});

		SetupIrisSupport(Target);
	}
}