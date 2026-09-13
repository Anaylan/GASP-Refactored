# Game Animation Sample Refactored (GASP)

Reworked and improved C++ architecture based on Epic's Game Animation Sample, powered by Mover 2.0 and NetworkPrediction.

## Architecture & Modules

The plugin is structured into four dedicated modules:

- **`GASP`** (Runtime): Core gameplay module containing the custom Mover component (`UGASPMoverComponent`), locomotion modes, obstacle traversal system (`UGASPTraversalComponent`), ragdoll simulation and tasks, multi-character interactions, foley audio subsystem (`UGASPFoleyWorldSubsystem`), and animation nodes.
- **`GASPCamera`** (Runtime): Gameplay camera interface (`IGASPGameplayCameraInterface`) and camera rig integration.
- **`GASPExtras`** (Runtime): Sample implementations including `AGASPCharacterExample`, player controller, input mappings, moving platforms, and test interactions (takedown, shove).
- **`GASPEditor`** (UncookedOnly): Editor tools, animation graph nodes (`UAnimGraphNode_GameplayTagsBlend`), and rig unit customizations.

## Key Features

- **Mover 2.0 Locomotion**: Built natively on Epic's Mover framework with `NetworkPrediction` support for client-side prediction, rollback, and replication.
- **Dual Locomotion Pipelines**: Runtime-switchable between Motion Matching (Pose Search + BlendStack + Trajectory Prediction) and State Machine pipelines via console variable or configuration.
- **Obstacle Traversal System**: Multi-stage trace detection for obstacle height, depth, front/back ledges, and floor clearance. Executes Chooser-selected animations with dynamic Motion Warping targets (vault, hurdle, climb).
- **Physics-Driven Ragdoll & Get-Up**: Full ragdoll integration with slope auto-roll torque, impact direction prediction, Physics Control curves, and directional rolling or standing get-up montages evaluated through Chooser tables.
- **Multi-Character Interactions**: Networked interaction framework (`UGASPCharacterInteractionComponent`) using server-authoritative Choosers and multi-character pose search for synchronized interactions.
- **Foley & Surface Audio**: World subsystem (`UGASPFoleyWorldSubsystem`) driving surface-dependent MetaSounds, decals, and Niagara effects triggered by animation notifies.
- **Overlay & Layering System**: GameplayTag-driven stance, gait, and overlay management with linked animation layers.
- **Procedural IK & Rig Units**: Custom Control Rig units for slope alignment, toe ground alignment, hyperextension prevention, and pelvis clamping.

## Supported Engine Versions & Platforms

| Plugin Version | Unreal Engine Version |
|----------------|-----------------------|
| [1.3](https://github.com/Anaylan/GASP-Refactored/releases/tag/1.3) | 5.5 |
| [1.11](https://github.com/Anaylan/GASP-Refactored/releases/tag/1.11) | 5.6 |
| [1.12](https://github.com/Anaylan/GASP-Refactored/releases/tag/1.12) | 5.7 |
| [2.0](https://github.com/Anaylan/GASP-Refactored/releases/tag/2.0) | 5.8 |

Target platform is **Windows (Win64)**.

## Prerequisites & Required Plugins

The plugin relies on the following built-in Unreal Engine plugins enabled in `GASP.uplugin`:
- `Mover`, `NetworkPrediction`, `GameplayTasks`
- `PoseSearch`, `MotionTrajectory`, `MotionWarping`, `AnimationWarping`
- `Chooser`, `BlendStack`
- `GameplayCameras`, `EnhancedInput`
- `PhysicsControl`, `GameplayInteractions`
- `AnimationLocomotionLibrary`, `DeformerGraph`, `RigLogic`, `CurveExpression`

## Quick Start

1. Clone or extract the repository into your project's `Plugins` directory.
2. Ensure the required engine plugins listed above are enabled in your project.
3. Configure `Config/DefaultNetworkPrediction.ini` as described below.
4. Compile and launch your project editor.

## Configuration

GASP requires independent ticking policy settings in `Config/DefaultNetworkPrediction.ini` to avoid movement jittering under NetworkPrediction:

```ini
[/Script/NetworkPrediction.NetworkPredictionSettingsObject]
Settings=(PreferredTickingPolicy=Independent,ReplicatedManagerClassOverride=None,FixedTickFrameRate=60,bForceEngineFixTickForcePhysics=True,SimulatedProxyNetworkLOD=Interpolated,FixedTickInterpolationBufferedMS=100,IndependentTickInterpolationBufferedMS=100,IndependentTickInterpolationMaxBufferedMS=250,FixedTickInputSendCount=6,IndependentTickInputSendCount=6,MaximumRemoteInputFaultLimit=6)
```

## Documentation & References

- [CONSOLE_COMMANDS.md](CONSOLE_COMMANDS.md) - Runtime console variables and debug commands reference.\

## License

Licensed under the MIT License. See [LICENSE.md](LICENSE.md) for details.