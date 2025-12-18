# Game Animation Sample Refactored

Completely reworked and improved С++ version of Game Animation Sample.

<details>

<summary><b>Features</b></summary>

- Game Animation Sample.
- Reworked plugin structure. Content is separated into 3 categories: `GASP` - main content, `GASPCamera` - camera-related content, and `GASPExtras` - other optional content.
- Overlay layering system built with separate Anim Graphs and Linked Layers.
- All overlays from ALS.
- Basic weapon attach system from ALS.
- Basic overlay switcher widget from ALS.

For more information, see the [Releases](https://github.com/Anaylan/GASP-Refactored/releases). Reading the changelogs is a good way to keep up to date with the newest features of a plugin.
</details>

## Supported Unreal Engine Versions & Platforms

| Plugin Version                                                     | Unreal Engine Version |
|--------------------------------------------------------------------|-----------------------|
| [1.3](https://github.com/Anaylan/GASP-Refactored/releases/tag/1.3) | 5.5                   |
| [1.11](https://github.com/Anaylan/GASP-Refactored/releases/tag/1.11) | 5.6                   |
| [1.12](https://github.com/Anaylan/GASP-Refactored/releases/tag/1.12) | 5.7                   |
| [2.0](https://github.com/Anaylan/GASP-Refactored/releases/tag/2.0) | 5.7                   |

**The plugin is developed and tested primarily on Windows, so use it on other platforms at your own risk.**

## Quick Start

1. Clone the repository to your project's `Plugins` folder, or download the latest release and extract it to your
   project's `Plugins` folder.
2. Recompile your project.

## Console Commands

GASP provides several console commands for debugging and configuration. See [CONSOLE_COMMANDS.md](CONSOLE_COMMANDS.md) for a complete list of available commands.

## License & Contribution

GASP Refactored is licensed under the MIT License, see [LICENSE.md](LICENSE.md) for more information. Other developers are encouraged to fork the repository, open issues & pull requests to help the development.