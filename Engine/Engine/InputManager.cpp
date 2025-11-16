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
#include "KeyCodes.h"
#include "GUI/ViewportPanel.h"
#include "FileIO/Settings.h"

namespace
{
  /* =================== //
  // Input state arrays  //
  // =================== */
  std::array<bool, GLFW_MOUSE_BUTTON_LAST + GLFW_KEY_LAST + 2> currentKeys;
  std::array<bool, GLFW_MOUSE_BUTTON_LAST + GLFW_KEY_LAST + 2> previousKeys;

  /* =================== //
  // Mouse state         //
  // =================== */
  glm::vec2 mousePosition;
  glm::vec2 scrollOffset;

  bool is_cursor_in_viewport = false;

  // Helper function to convert KeyCode to array index
  inline int key_to_index(Engine::KeyCode key)
  {
    int keyInt = static_cast<int>(key);

    // Mouse buttons: 0-7 → index 0-7
    if (keyInt <= GLFW_MOUSE_BUTTON_LAST)
    {
      return keyInt;
    }
    // Keyboard keys: offset by (GLFW_MOUSE_BUTTON_LAST + 1)
    else
    {
      return keyInt + GLFW_MOUSE_BUTTON_LAST + 1; // Adjust for GLFW key offset
    }
  }
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

    GLFWwindow* window = Engine::WindowManager::get_window();

    // Update previous states
    previousKeys = currentKeys;

    // Update current mouse button state
    for (int button = 0; button <= GLFW_MOUSE_BUTTON_LAST; ++button)
    {
      currentKeys[button] = (glfwGetMouseButton(window, button) == GLFW_PRESS);
    }

    // Update current keyboard state
    for (int key = 0; key <= GLFW_KEY_LAST; ++key)
    {
      currentKeys[key + GLFW_MOUSE_BUTTON_LAST + 1] = (glfwGetKey(window, key) == GLFW_PRESS);
    }


    // Update mouse position
    f64 xpos, ypos;
    glfwGetCursorPos(window, &xpos, &ypos);

    //std::cout << "Raw Mouse Position: (" << xpos << ", " << ypos << ")\n";

    // Check if imgui viewport mode is enabled to determine mouse position calculation
    // auto [mousePosition, is_in_viewport] = Engine::Graphics::screen_to_viewport({ static_cast<f32>(xpos), static_cast<f32>(ypos) });
    auto result = Engine::Graphics::screen_to_viewport({ static_cast<f32>(xpos), static_cast<f32>(ypos) });
    mousePosition = result.first;
    is_cursor_in_viewport = result.second;

    //std::cout << "Mouse Position: (" << mousePosition.x << ", " << mousePosition.y << ")\n";

    Engine::Perf::stop("Input");
  }

  bool Input::is_pressed(KeyCode key)
  {
    int idx = key_to_index(key);
    return currentKeys[idx] && !previousKeys[idx];
  }

  bool Input::is_released(KeyCode key)
  {
    int idx = key_to_index(key);
    return !currentKeys[idx] && previousKeys[idx];
  }

  bool Input::is_held(KeyCode key)
  {
    int idx = key_to_index(key);
    return currentKeys[idx];
  }

  glm::vec2 Input::get_mouse_position()
  {
    return mousePosition;
  }

  std::optional<glm::vec2> Input::get_mouse_position_if_in_viewport()
  {
    if (is_cursor_in_viewport)
      return mousePosition;
    else
      return std::nullopt;
  }

  glm::vec2 Input::get_mouse_world_position()
  {
    return Engine::Graphics::virtual_to_world(mousePosition);
  }

  std::optional<glm::vec2> Input::get_mouse_world_position_if_in_viewport()
  {
    if (is_cursor_in_viewport)
      return Engine::Graphics::virtual_to_world(mousePosition);
    else
      return std::nullopt;
  }

  glm::vec2 Input::get_scroll_offset()
  {
    glm::vec2 result = scrollOffset;
    scrollOffset = { 0.0f, 0.0f }; // Reset after reading
    return result;
  }

  void Input::set_scroll_offset(f64 xoffset, f64 yoffset)
  {
    //std::cout << "Scroll Offset Callback: (" << xoffset << ", " << yoffset << ")\n";
    scrollOffset = { static_cast<f32>(xoffset), static_cast<f32>(yoffset) };
  }

  bool Input::is_in_viewport()
  {
    return is_cursor_in_viewport;
  }
}
