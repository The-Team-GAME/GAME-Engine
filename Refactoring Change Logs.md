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
Required components to create a Rect entity:

```cpp
Engine::ECS::create_entity()
    .add_component<Components::Transform2D>(x, y, rotation, size, size)
    .add_component<Components::Color>(color)
    .add_component<Components::Texture>(Engine::Resources::get_rect_texture());
    //   ^^^ Emphasis on `Engine::Resources::get_rect_texture()` !!! ^^^
```

> **Note**: The reason why theres no Rect component is so that we can centralize drawing into `StaticSpriteRenderSystem`. This means there is no need for a separate rect-drawing system.



### Resources System
#### Resource path management
  - Set base path once using: `Engine::Resources::set_resource_path("Assets/")`
  - Load resources using relative paths: `load_texture("images/player.png")` instead of `load_texture("Assets/images/player.png")`
  - This will make it easier to integrate more file drop features (automatically copy into the Game Dev's project folder)

Example:
```cpp
// Set the base resource path (typically in game initialization)
Engine::Resources::set_resource_path("Assets/");

// Load resources - automatically cached and reference counted
auto texture = Engine::Resources::load_texture("images/player.png");
auto font = Engine::Resources::load_font("fonts/arial.ttf", 24);
auto sound = Engine::Resources::load_sound("sounds/move1.wav");
auto music = Engine::Resources::load_music("sounds/move1.ogg"); // Streamed from disk
```

#### Resource Lifecycle

```cpp
// First load - reads from disk and caches
auto tex1 = Engine::Resources::load_texture("player.png");

// Subsequent loads - returns cached resource (instant)
auto tex2 = Engine::Resources::load_texture("player.png"); // Same resource

// When all references are released, resource is automatically freed
// No need to worry about memory leak because we don't have to unload manually anymore yippee!
```

#### Special Resources

- **Unit Rectangle Texture**: `Engine::Resources::get_rect_texture()`
  - 1x1 white pixel texture
  - Used for rendering colored rectangles


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

> ⚠️ **Warning**: Do NOT use this include inside `Engine/` directory as it may cause circular dependency issues


<sub> **Side note:** I will check with Prof if we can include our engine code in the project additional path so we can just do `#include <MonkeyEngine.h>`</sub>
 