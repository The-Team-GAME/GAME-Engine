#pragma once
#pragma once
#include <glm/glm.hpp>

namespace Engine
{
  class Input
  {
  public:
    static void Update();

    // Keyboard input
    static bool IsKeyPressed(int key);
    static bool IsKeyReleased(int key);
    static bool IsKeyHeld(int key);

    // Mouse input
    static bool IsMouseButtonPressed(int button);
    static bool IsMouseButtonReleased(int button);
    static bool IsMouseButtonHeld(int button);
    static glm::vec2 GetMousePosition();
  };
}