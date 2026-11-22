# Monkey Engine

A custom 2D C++ game engine and editor built by **Team Monkey Stackers** for the game **Monkey Stole My GPA**.

## Features

- Entity Component System (ECS) with layers and component reflection
- Batched OpenGL renderer with sprite sheet animation, custom shaders, and font rendering (FreeType)
- Physics and collision with quad tree spatial partitioning
- Audio via FMOD / FMOD Studio
- Lua scripting (Sol2)
- JSON serialization for levels, prefabs, tilemaps, and settings (RapidJSON)
- ImGui editor: entity editor, prefab editor, tilemap editor, level manager, resource manager, performance viewer

## Project Structure

```
Engine/
  Engine/      Core engine (ECS, graphics, audio, physics, editor, scripting)
  GameLogic/   Game-specific states, components, and systems
  Assets/      Textures, sounds, fonts, shaders, levels, prefabs, settings
Dependencies/  Third-party libraries (GLFW, GLEW, GLM, ImGui, FMOD, FreeType, Lua, ...)
benchmarks/    ECS traversal benchmark (see benchmarks/README.md)
tools/         Python helper scripts for asset/level maintenance
```

## Building

**Requirements:** Windows, Visual Studio 2022 (v143 toolset), x64.

1. Open `GAME.sln` in Visual Studio.
2. Pick a configuration: `Debug`, `Release`, or `Game` (standalone game build, no editor).
3. Build and run. No additional setup is needed.

To build the standalone game from the command line:

```powershell
.\build_game.ps1                  # defaults to Game|x64
.\build_game.ps1 -Configuration Release
```

Output is written to `x64/<Configuration>/`.

## Keybinds

| Key | Action |
| --- | --- |
| F1 | Toggle ImGui windows |
| F2 | Toggle viewport mode |
| F3 | Toggle debug visualization |
| F10 | Toggle FPS overlay |
| F11 | Toggle fullscreen |
| P | Pause / play |
| ] | Step one frame while paused |
| Middle mouse drag | Free camera |
| Scroll | Zoom |

## Documentation

- `editorGuide.pdf`: editor user guide
- `MonkeyStackers_ReadMe.txt`: detailed feature list and demo states
- `projectGuide.txt`: quick build notes
