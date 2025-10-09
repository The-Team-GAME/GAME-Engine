### Major Refactoring Change Log:

- Time.h: added get_time
- Math library: 
  - added conversion to glm equivalent
  - added Reflection
- Engine managers are now all namespaces, no more global constants!!
- ImGui: 
  - docking enabled
  - precompiled to increase compile time and reduce clutter in the solution (now if you try to find all in the Entire Solution imgui files will no longer show up)
- Engine:
  - Core systems are now automatically registered in Core.cpp (in the function register_engine_systems)
- Graphics:
  - From now on, to the minimum required components to create a Rect entity are, 
    ```
    Engine::ECS::create_entity()
      .add_component<Components::Transform2D>(x, y, rotation, size, size)
      .add_component<Components::Color>(color)
      .add_component<Components::Texture>(Engine::Resources::get_rect_texture())

      // Emphasis on the Engine::Resources::get_rect_texture()
    ```
    this centralises all the drawing systems into StaticSpriteRenderSystem, so theres no need for a seperate system just for drawing rects which we will rarely be used in our game anyway
- Reflection: still WIP, when everything is finalized i will update again


### Important notes:

From now on, all GameLogic code should be inside the `GameLogic`, folder (meaning things that should not be built with the engine like ur game states etc.)

In gamelogic side, you can now just include just this to get everything
```
#include "../../Engine/MonkeyEngine.h"
```
Dont use this inside `Engine/` or you will get circular dependencies issues

Side note:
I will check with prof if we can include our engine code in the  project additional path so we can just do 
```
#include <MonkeyEngine.h>
```




<!-- 
### Reflection:

If you want to make your struct/component work automatically with reflection,
after you define your struct, just use the REFLECT macro

```
struct Transform2D
{

  Math::Vec2D position;
  float rotation = 0.0f; // In degrees
  Math::Vec2D scale = { 1.0f, 1.0f };
};
REFLECT(Transform2D, position, rotation, scale)
```

-->