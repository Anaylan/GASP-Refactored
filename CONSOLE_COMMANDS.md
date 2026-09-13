# GASP Console Variables & Commands Reference

Console variables provided by the GASP plugin for runtime configuration, tuning, and debug visualization.

## Character & Control

| Command | Type | Default | Description |
|---|---|---|---|
| `gasp.movement.style.aim` | `int32` | `0` | Direction threshold profile index for Aim rotation mode. |
| `gasp.movement.style.strafe` | `int32` | `1` | Direction threshold profile index for Strafe rotation mode. |
| `gasp.control.style` | `int32` | `0` | Control style: `0` = Default (orient to controller/velocity), `1` = TwinStick. |
| `gasp.analoginput` | `int32` | `0` | Analog movement input handling. Values `> 1` require input magnitude to meet `AnalogMovementThreshold` for full movement. |
| `gasp.physics.profile` | `int32` | `0` | Active physics control profile index for character physics simulation. |

## Animation & Locomotion

| Command | Type | Default | Description |
|---|---|---|---|
| `gasp.locomotion.style` | `int32` | `1` | Locomotion pipeline: `0` = Motion Matching (BlendStack), `1` = State Machine. |
| `gasp.offsetrootbone.enabled` | `bool` | `1` | Enables root bone offset calculation in the animation instance (`1` = On, `0` = Off). |
| `gasp.footplacement.enabled` | `bool` | `1` | Enables procedural foot placement IK (`1` = On, `0` = Off). |
| `gasp.motionmatching.LOD` | `int32` | `0` | LOD level for the motion matching pose search database. |

## Traversal

| Command | Type | Default | Build Scope | Description |
|---|---|---|---|---|
| `gasp.traversal.DrawDebugLevel` | `int32` | `0` | Editor (`WITH_EDITOR`) | Traversal debug visualization: `0` = Disabled, `1` = Hit & ledge checks, `2` = Full obstacle geometry and metrics. |
| `gasp.traversal.DrawDebugDuration` | `float` | `0.0` | Editor (`WITH_EDITOR`) | Lifetime in seconds for traversal debug drawings (`0.0` = single frame). |

## Ragdoll Simulation

| Command | Type | Default | Build Scope | Description |
|---|---|---|---|---|
| `gasp.ragdoll.drawdebug` | `bool` | `0` | `ENABLE_DRAW_DEBUG` | Draws the ragdoll pelvis target coordinate system in `MovementMode_Ragdolling`. |
| `gasp.ragdoll.drawdebugduration` | `float` | `0.0` | `ENABLE_DRAW_DEBUG` | Lifetime in seconds of ragdoll movement debug shapes (`0.0` = single frame). |
| `gasp.ragdoll.task.drawdebug` | `bool` | `0` | `ENABLE_DRAW_DEBUG` | Draws predicted ground impact traces for active `UCharacterTask_Ragdoll`. |

## Foley Audio & Visual Logger

| Command | Type | Default | Description |
|---|---|---|---|
| `gasp.DrawVisLogShapesForFoleySounds.enabled` | `bool` | `0` | Enables Visual Logger debug shape recording for foley sound surface traces. |

## On-Screen Character Diagnostics

| Command | Type | Default | Description |
|---|---|---|---|
| `gasp.Debug.DrawCharacterShapes` | `int32` | `0` | Toggles rendering of character collision capsule, mesh relative bounds, and heading vectors. |
| `gasp.Debug.DrawCharacterStates` | `int32` | `0` | Displays character movement mode, stance, gait, rotation mode, and active task states on screen. |
| `gasp.Debug.DrawCharacterGraphs` | `int32` | `0` | Draws trajectory history and velocity trend graphs. |

## Usage Notes

- In Unreal Engine console (press `~` or `` ` ``), enter the command followed by the desired value (e.g. `gasp.locomotion.style 0`).
- Boolean flags accept `1` for true/enabled and `0` for false/disabled.
- Commands designated as Editor-only or requiring `ENABLE_DRAW_DEBUG` are stripped in Shipping configurations.
