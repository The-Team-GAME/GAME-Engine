# Engine Refactoring Changelog

## Major Changes

### Core Systems
- **Time.h**: Added `get_time()` function
- **Math Library**: 
  - Added conversion to GLM equivalent types
  - Added Reflection support
- **Engine Managers**: Now implemented as namespaces instead of global constants
- **Engine Core**: Core systems are now automatically registered in `Core.cpp` via the `register_engine_systems()` function

### ImGui Integration
- Docking is now enabled
- Precompiled to improve compile times and reduce solution clutter
  - ImGui files no longer appear when using "Find All in Entire Solution"

### Graphics System
Minimum required components to create a Rect entity:

```cpp
Engine::ECS::create_entity()
    .add_component<Components::Transform2D>(x, y, rotation, size, size)
    .add_component<Components::Color>(color)
    .add_component<Components::Texture>(Engine::Resources::get_rect_texture());
```

> **Note**: Use `Engine::Resources::get_rect_texture()` to centralize drawing into `StaticSpriteRenderSystem`. This eliminates the need for a separate rect-drawing system.

### Reflection System
⚠️ **Work in Progress** - Documentation will be updated once finalized

---

## Important Notes

### Project Structure
- All game logic code should now be placed in the `GameLogic/` folder
- This includes game states and other game-specific code that should not be built with the engine

### Include Path Updates
**For GameLogic code**, you can just include the engine header like this:
```cpp
#include "../../Engine/MonkeyEngine.h"
```
Don't have to manually include one by one anymore

> ⚠️ **Warning**: Do NOT use this include inside `Engine/` directory as it will cause circular dependency issues


**Side note:** I will check with prof if we can include our engine code in the  project additional path so we can just do 
```
#include <MonkeyEngine.h>
```
