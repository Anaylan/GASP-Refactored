# GASP Refactored

A C++ rewrite of Epic's **Game Animation Sample (GASP)**, rebuilt on **Mover 2.0** with **NetworkPrediction** and restructured into a modular, designer-friendly plugin.

<!-- TODO: add a 1-2 minute demo video or GIF here, e.g. ![Demo](docs/demo.gif) -->

## Why this project

Epic's Game Animation Sample is a great showcase, but it is Blueprint-heavy and ships as a sample project rather than a reusable plugin. This project turns it into a modular, networked C++ plugin that you can drop into any project.

## What's different from Epic's sample

- **Everything rewritten in C++** and reorganized into a modular plugin.
- **Mover 2.0 + NetworkPrediction in C++**, with a custom `UGASPMoverComponent` and locomotion modes.
- **Ragdoll rebuilt on GameplayTasks.**
- **Overlay layering system** built with separate Anim Graphs and Linked Layers.
- **ALS overlays, weapon attach and overlay switcher widget** ported into the plugin.

The earlier Character Movement Component version of this plugin is preserved on the `dev/cmc` branch.

## Key features

- **Networked locomotion**: built on Mover 2.0 and NetworkPrediction with client-side prediction and replication.
- **Switchable locomotion pipelines**: Motion Matching and State Machine, selectable at runtime.
- **Character systems**: traversal, ragdoll, multi-character interactions and more.
- **Layered animation**: GameplayTag-driven overlays built with linked animation layers.
- **Audio and procedural animation**: surface-aware foley and Control Rig-based IK.

## Architecture

| Module | Type | Purpose |
|---|---|---|
| `GASP` | Runtime | Core gameplay and animation systems |
| `GASPCamera` | Runtime | Camera integration |
| `GASPExtras` | Runtime | Optional sample content and example implementations |
| `GASPEditor` | UncookedOnly | Editor tools and customizations |

## Supported engine versions

| Plugin version | Unreal Engine |
|---|---|
| [1.3](https://github.com/Anaylan/GASP-Refactored/releases/tag/1.3) | 5.5 |
| [1.11](https://github.com/Anaylan/GASP-Refactored/releases/tag/1.11) | 5.6 |
| [1.12](https://github.com/Anaylan/GASP-Refactored/releases/tag/1.12) | 5.7 |
| [2.0](https://github.com/Anaylan/GASP-Refactored/releases/tag/2.0) | 5.8 |

Target platform: **Windows (Win64)**. Other platforms are untested.

## Quick start

1. Clone the repository (or extract a release) into your project's `Plugins` folder.
2. Make sure the engine plugins listed in `GASP.uplugin` are enabled.
3. Add the NetworkPrediction settings below to `Config/DefaultNetworkPrediction.ini`.
4. Recompile and open the editor.

## Troubleshooting

**Movement jitter under NetworkPrediction.** GASP needs the independent ticking policy. Add this to `Config/DefaultNetworkPrediction.ini`:

```ini
[/Script/NetworkPrediction.NetworkPredictionSettingsObject]
Settings=(PreferredTickingPolicy=Independent,ReplicatedManagerClassOverride=None,FixedTickFrameRate=60,bForceEngineFixTickForcePhysics=True,SimulatedProxyNetworkLOD=Interpolated,FixedTickInterpolationBufferedMS=100,IndependentTickInterpolationBufferedMS=100,IndependentTickInterpolationMaxBufferedMS=250,FixedTickInputSendCount=6,IndependentTickInputSendCount=6,MaximumRemoteInputFaultLimit=6)
```

## Console commands

Runtime debug and configuration commands are listed in [CONSOLE_COMMANDS.md](CONSOLE_COMMANDS.md).

## Credits

- **Epic Games**: Game Animation Sample, including animation and mesh assets.
- **ALS (Advanced Locomotion System)**: overlay set, weapon attach and overlay switcher widget. <!-- TODO: link the exact ALS repository/author -->

## License

Code is released under the MIT License, see [LICENSE.md](LICENSE.md). Epic's original assets remain subject to Epic's own license terms.

## Contributing

Forks, issues and pull requests are welcome.
