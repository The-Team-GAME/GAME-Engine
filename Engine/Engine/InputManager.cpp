//-----------------------------------------------------------------------------------
// file:    InputManager.cpp
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   This file implements the InputManager which handles keyboard and mouse
//          input state tracking. Provides frame-based input detection (pressed,
//          released, held) and mouse position in virtual coordinates.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#include "InputManager.h"
#include "Core.h"
#include "Precompiled.h"
#include <array>
#include <glm/glm.hpp>
#include "Perf.h"
#include "WindowManager.h"
#include "Graphics/Renderer.h"

namespace
{
  /* =================== //
  // Input state arrays  //
  // =================== */
  std::array<bool, GLFW_KEY_LAST + 1> currentKeys;
  std::array<bool, GLFW_KEY_LAST + 1> previousKeys;

  // Mouse button state
  std::array<bool, GLFW_MOUSE_BUTTON_LAST + 1> currentMouseButtons;
  std::array<bool, GLFW_MOUSE_BUTTON_LAST + 1> previousMouseButtons;

  /* =================== //
  // Mouse state         //
  // =================== */
  glm::vec2 mousePosition;
}

namespace Engine
{
  /*=================================================================================
  Function Brief:
  Updates the internal state of the input manager. Should be called once per frame
  before checking any input states. Updates keyboard, mouse button states, and
  mouse position in virtual coordinates.
  ==================================================================================*/
  void Input::update()
  {
    Engine::Perf::start("Input");

    GLFWwindow* window = Engine::WindowManager::get_window(); //TODO: pass window as parameter in constructor

    // Update previous states
    previousKeys = currentKeys;
    previousMouseButtons = currentMouseButtons;

    // Update current keyboard state
    for (int key = 0; key <= GLFW_KEY_LAST; ++key)
    {
      currentKeys[key] = (glfwGetKey(window, key) == GLFW_PRESS);
    }

    // Update current mouse button state
    for (int button = 0; button <= GLFW_MOUSE_BUTTON_LAST; ++button)
    {
      currentMouseButtons[button] = (glfwGetMouseButton(window, button) == GLFW_PRESS);
    }

    // Update mouse position
    f64 xpos, ypos;
    glfwGetCursorPos(window, &xpos, &ypos);

    //std::cout << "Raw Mouse Position: (" << xpos << ", " << ypos << ")\n";

    // Convert from window coordinates to virtual coordinates using Graphics system
    mousePosition = Engine::Graphics::screen_to_virtual({ static_cast<f32>(xpos), static_cast<f32>(ypos) });

    //std::cout << "Mouse Position: (" << mousePosition.x << ", " << mousePosition.y << ")\n";

    Engine::Perf::stop("Input");
  }

  bool Input::is_key_pressed(s32 key)
  {
    return currentKeys[key] && !previousKeys[key];
  }

  bool Input::is_key_released(s32 key)
  {
    return !currentKeys[key] && previousKeys[key];
  }

  bool Input::is_key_held(s32 key)
  {
    return currentKeys[key];
  }

  bool Input::is_mouse_button_pressed(s32 button)
  {
    return currentMouseButtons[button] && !previousMouseButtons[button];
  }

  bool Input::is_mouse_button_released(s32 button)
  {
    return !currentMouseButtons[button] && previousMouseButtons[button];
  }

  bool Input::is_mouse_button_held(s32 button)
  {
    return currentMouseButtons[button];
  }

  glm::vec2 Input::GetMousePosition()
  {
    return mousePosition;
  }

  glm::vec2 Input::GetMouseWorldPosition()
  {
    return Engine::Graphics::virtual_to_world(mousePosition);
  }
}
