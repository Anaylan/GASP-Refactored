# GASP Console Commands

This document lists all available console commands for the GASP (Game Animation Sample Refactored) plugin.

## Character Commands

### Strafe style
- **`gasp.movement.style.strafe`** (int32, default: 0)
  - Sets threshold style for strafe rotation mode
  - Usage: `gasp.movement.style.strafe 2`

### Aim style
- **`gasp.movement.style.aim`** (int32, default: 0)
  - Sets threshold style for aim rotation mode
  - Usage: `gasp.movement.style.aim 2`

### Control style
- **`gasp.control.style`** (int32, default: 0)
  - Sets control style default\twinstick
  - Usage: `gasp.control.style 1`

## Animation Commands

### State Machine
- **`gasp.locomotion.style`** (int, default: 0)
  - Switch locomotion style: motion matching\state machine
  - Usage: `gasp.locomotion.style 1`

### Root Bone Offset
- **`gasp.offsetrootbone.enabled`** (bool, default: false)
  - Enables root bone offset functionality
  - Usage: `gasp.offsetrootbone.enabled 1`

### Motion Matching
- **`gasp.motionmatching.LOD`** (int32, default: 0)
  - Sets LOD level for motion matching database
  - Usage: `gasp.motionmatching.LOD 2`


## Traversal Commands

### Debug Visualization
- **`gasp.traversal.DrawDebugLevel.enabled`** (int32, default: 0)
  - Sets debug level for traversal system
  - Usage: `gasp.traversal.DrawDebugLevel.enabled 1`

- **`gasp.traversal.DrawDebugDuration.enabled`** (float, default: 0.0)
  - Sets debug duration for traversal visualization
  - Usage: `gasp.traversal.DrawDebugDuration.enabled 5.0`

## Foley Audio Commands

### Debug Visualization
- **`gasp.DrawVisLogShapesForFoleySounds.enabled`** (bool, default: false)
  - Enables debug visualization for foley sound shapes
  - Usage: `gasp.DrawVisLogShapesForFoleySounds.enabled 1`

## How to Use

1. Open the console in Unreal Engine (press `~` or `'` key)
2. Type any of the commands above
3. Press Enter to execute

## Notes

- All commands are prefixed with `gasp.` to avoid conflicts with other plugins
- Boolean values: `1` = true, `0` = false
- Some commands are only available in editor builds (`WITH_EDITOR`)
- Changes take effect immediately unless specified otherwise

## Troubleshooting

If a command doesn't work:
1. Make sure the GASP plugin is loaded
2. Check if you're in the correct build configuration (some commands are editor-only)
3. Verify the command syntax and parameter types
