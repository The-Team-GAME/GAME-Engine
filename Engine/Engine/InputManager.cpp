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
#include "Logger.h"
#include <array>
#include <cmath>
#include <glm/glm.hpp>
#include "Perf.h"
#include "WindowManager.h"
#include "Graphics/Renderer.h"
#include "KeyCodes.h"
#include "GUI/ViewportPanel.h"
#include "FileIO/Settings.h"
#include "ImGuiManager.h"
#include "Precompiled.h"

namespace
{
  constexpr int kInputStateCount = GLFW_MOUSE_BUTTON_LAST + GLFW_KEY_LAST + 2;
  constexpr int kGamepadButtonCount = GLFW_GAMEPAD_BUTTON_LAST + 1;
  constexpr int kGamepadAxisCount = GLFW_GAMEPAD_AXIS_LAST + 1;
  constexpr float kStickPressThreshold = 0.55f;
  constexpr float kStickReleaseThreshold = 0.35f;

  enum class GamepadDirection : size_t
  {
    Left = 0,
    Right,
    Up,
    Down,
    Count
  };

  constexpr size_t gamepad_direction_index(GamepadDirection direction)
  {
    return static_cast<size_t>(direction);
  }

  /* =================== //
  // Input state arrays  //
  // =================== */
  std::array<bool, kInputStateCount> currentKeys;
  std::array<bool, kInputStateCount> previousKeys;
  std::array<bool, kGamepadButtonCount> currentGamepadButtons;
  std::array<bool, kGamepadButtonCount> previousGamepadButtons;
  std::array<float, kGamepadAxisCount> currentGamepadAxes;
  std::array<float, kGamepadAxisCount> previousGamepadAxes;
  std::array<bool, gamepad_direction_index(GamepadDirection::Count)> currentGamepadDirections;
  std::array<bool, gamepad_direction_index(GamepadDirection::Count)> previousGamepadDirections;
  bool any_key_pressed_this_frame = false;

  /* =================== //
  // Mouse state         //
  // =================== */
  glm::vec2 mousePosition;
  glm::vec2 scrollOffset;

  inline bool gamepad_axis_active(float axis_value, bool was_active, bool positive_direction)
  {
    float signed_value = positive_direction ? axis_value : -axis_value;
    float threshold = was_active ? kStickReleaseThreshold : kStickPressThreshold;
    return signed_value >= threshold;
  }

  void update_gamepad_state()
  {
    previousGamepadButtons = currentGamepadButtons;
    previousGamepadAxes = currentGamepadAxes;
    previousGamepadDirections = currentGamepadDirections;

    currentGamepadButtons.fill(false);
    currentGamepadAxes.fill(0.0f);
    currentGamepadDirections.fill(false);

    GLFWgamepadstate state{};
    for (int joystick = GLFW_JOYSTICK_1; joystick <= GLFW_JOYSTICK_LAST; ++joystick)
    {
      if (!glfwJoystickIsGamepad(joystick))
      {
        continue;
      }

      if (!glfwGetGamepadState(joystick, &state))
      {
        continue;
      }

      for (int button = 0; button < kGamepadButtonCount; ++button)
      {
        currentGamepadButtons[button] = (state.buttons[button] == GLFW_PRESS);
      }

      for (int axis = 0; axis < kGamepadAxisCount; ++axis)
      {
        currentGamepadAxes[axis] = state.axes[axis];
      }

      currentGamepadDirections[gamepad_direction_index(GamepadDirection::Left)] =
        gamepad_axis_active(currentGamepadAxes[GLFW_GAMEPAD_AXIS_LEFT_X],
          previousGamepadDirections[gamepad_direction_index(GamepadDirection::Left)], false);
      currentGamepadDirections[gamepad_direction_index(GamepadDirection::Right)] =
        gamepad_axis_active(currentGamepadAxes[GLFW_GAMEPAD_AXIS_LEFT_X],
          previousGamepadDirections[gamepad_direction_index(GamepadDirection::Right)], true);
      currentGamepadDirections[gamepad_direction_index(GamepadDirection::Up)] =
        gamepad_axis_active(currentGamepadAxes[GLFW_GAMEPAD_AXIS_LEFT_Y],
          previousGamepadDirections[gamepad_direction_index(GamepadDirection::Up)], false);
      currentGamepadDirections[gamepad_direction_index(GamepadDirection::Down)] =
        gamepad_axis_active(currentGamepadAxes[GLFW_GAMEPAD_AXIS_LEFT_Y],
          previousGamepadDirections[gamepad_direction_index(GamepadDirection::Down)], true);
      return;
    }
  }

  inline bool is_gamepad_button_held(int button)
  {
    return currentGamepadButtons[button];
  }

  inline bool is_gamepad_button_pressed(int button)
  {
    return currentGamepadButtons[button] && !previousGamepadButtons[button];
  }

  inline bool is_gamepad_button_released(int button)
  {
    return !currentGamepadButtons[button] && previousGamepadButtons[button];
  }

  inline bool is_gamepad_direction_held(GamepadDirection direction)
  {
    return currentGamepadDirections[gamepad_direction_index(direction)];
  }

  inline bool is_gamepad_direction_pressed(GamepadDirection direction)
  {
    size_t index = gamepad_direction_index(direction);
    return currentGamepadDirections[index] && !previousGamepadDirections[index];
  }

  inline bool is_gamepad_direction_released(GamepadDirection direction)
  {
    size_t index = gamepad_direction_index(direction);
    return !currentGamepadDirections[index] && previousGamepadDirections[index];
  }

  bool is_controller_mapping_held(Engine::KeyCode key)
  {
    using Engine::KeyCode;

    switch (key)
    {
    case KeyCode::A:
      return is_gamepad_direction_held(GamepadDirection::Left);
    case KeyCode::D:
      return is_gamepad_direction_held(GamepadDirection::Right);
    case KeyCode::Left:
      return is_gamepad_direction_held(GamepadDirection::Left) || is_gamepad_button_held(GLFW_GAMEPAD_BUTTON_DPAD_LEFT);
    case KeyCode::Right:
      return is_gamepad_direction_held(GamepadDirection::Right) || is_gamepad_button_held(GLFW_GAMEPAD_BUTTON_DPAD_RIGHT);
    case KeyCode::Up:
      return is_gamepad_direction_held(GamepadDirection::Up) || is_gamepad_button_held(GLFW_GAMEPAD_BUTTON_DPAD_UP);
    case KeyCode::Down:
      return is_gamepad_direction_held(GamepadDirection::Down) || is_gamepad_button_held(GLFW_GAMEPAD_BUTTON_DPAD_DOWN);
    case KeyCode::Space:
    case KeyCode::Enter:
      return is_gamepad_button_held(GLFW_GAMEPAD_BUTTON_A);
    case KeyCode::Escape:
      return is_gamepad_button_held(GLFW_GAMEPAD_BUTTON_START);
    case KeyCode::Tab:
      return is_gamepad_button_held(GLFW_GAMEPAD_BUTTON_DPAD_UP);
    case KeyCode::I:
      return is_gamepad_button_held(GLFW_GAMEPAD_BUTTON_BACK);
    case KeyCode::E:
    case KeyCode::F:
      return is_gamepad_button_held(GLFW_GAMEPAD_BUTTON_LEFT_BUMPER);
    case KeyCode::J:
      return is_gamepad_button_held(GLFW_GAMEPAD_BUTTON_X);
    case KeyCode::K:
      return is_gamepad_button_held(GLFW_GAMEPAD_BUTTON_Y);
    case KeyCode::L:
      return is_gamepad_button_held(GLFW_GAMEPAD_BUTTON_B);
    case KeyCode::ShiftLeft:
    case KeyCode::ShiftRight:
      return is_gamepad_button_held(GLFW_GAMEPAD_BUTTON_RIGHT_BUMPER);
    default:
      return false;
    }
  }

  bool is_controller_mapping_pressed(Engine::KeyCode key)
  {
    using Engine::KeyCode;

    switch (key)
    {
    case KeyCode::A:
      return is_gamepad_direction_pressed(GamepadDirection::Left);
    case KeyCode::D:
      return is_gamepad_direction_pressed(GamepadDirection::Right);
    case KeyCode::Left:
      return is_gamepad_direction_pressed(GamepadDirection::Left) || is_gamepad_button_pressed(GLFW_GAMEPAD_BUTTON_DPAD_LEFT);
    case KeyCode::Right:
      return is_gamepad_direction_pressed(GamepadDirection::Right) || is_gamepad_button_pressed(GLFW_GAMEPAD_BUTTON_DPAD_RIGHT);
    case KeyCode::Up:
      return is_gamepad_direction_pressed(GamepadDirection::Up) || is_gamepad_button_pressed(GLFW_GAMEPAD_BUTTON_DPAD_UP);
    case KeyCode::Down:
      return is_gamepad_direction_pressed(GamepadDirection::Down) || is_gamepad_button_pressed(GLFW_GAMEPAD_BUTTON_DPAD_DOWN);
    case KeyCode::Space:
    case KeyCode::Enter:
      return is_gamepad_button_pressed(GLFW_GAMEPAD_BUTTON_A);
    case KeyCode::Escape:
      return is_gamepad_button_pressed(GLFW_GAMEPAD_BUTTON_START);
    case KeyCode::Tab:
      return is_gamepad_button_pressed(GLFW_GAMEPAD_BUTTON_DPAD_UP);
    case KeyCode::I:
      return is_gamepad_button_pressed(GLFW_GAMEPAD_BUTTON_BACK);
    case KeyCode::E:
    case KeyCode::F:
      return is_gamepad_button_pressed(GLFW_GAMEPAD_BUTTON_LEFT_BUMPER);
    case KeyCode::J:
      return is_gamepad_button_pressed(GLFW_GAMEPAD_BUTTON_X);
    case KeyCode::K:
      return is_gamepad_button_pressed(GLFW_GAMEPAD_BUTTON_Y);
    case KeyCode::L:
      return is_gamepad_button_pressed(GLFW_GAMEPAD_BUTTON_B);
    case KeyCode::ShiftLeft:
    case KeyCode::ShiftRight:
      return is_gamepad_button_pressed(GLFW_GAMEPAD_BUTTON_RIGHT_BUMPER);
    default:
      return false;
    }
  }

  bool is_controller_mapping_released(Engine::KeyCode key)
  {
    using Engine::KeyCode;

    switch (key)
    {
    case KeyCode::A:
      return is_gamepad_direction_released(GamepadDirection::Left);
    case KeyCode::D:
      return is_gamepad_direction_released(GamepadDirection::Right);
    case KeyCode::Left:
      return is_gamepad_direction_released(GamepadDirection::Left) || is_gamepad_button_released(GLFW_GAMEPAD_BUTTON_DPAD_LEFT);
    case KeyCode::Right:
      return is_gamepad_direction_released(GamepadDirection::Right) || is_gamepad_button_released(GLFW_GAMEPAD_BUTTON_DPAD_RIGHT);
    case KeyCode::Up:
      return is_gamepad_direction_released(GamepadDirection::Up) || is_gamepad_button_released(GLFW_GAMEPAD_BUTTON_DPAD_UP);
    case KeyCode::Down:
      return is_gamepad_direction_released(GamepadDirection::Down) || is_gamepad_button_released(GLFW_GAMEPAD_BUTTON_DPAD_DOWN);
    case KeyCode::Space:
    case KeyCode::Enter:
      return is_gamepad_button_released(GLFW_GAMEPAD_BUTTON_A);
    case KeyCode::Escape:
      return is_gamepad_button_released(GLFW_GAMEPAD_BUTTON_START);
    case KeyCode::Tab:
      return is_gamepad_button_released(GLFW_GAMEPAD_BUTTON_DPAD_UP);
    case KeyCode::I:
      return is_gamepad_button_released(GLFW_GAMEPAD_BUTTON_BACK);
    case KeyCode::E:
    case KeyCode::F:
      return is_gamepad_button_released(GLFW_GAMEPAD_BUTTON_LEFT_BUMPER);
    case KeyCode::J:
      return is_gamepad_button_released(GLFW_GAMEPAD_BUTTON_X);
    case KeyCode::K:
      return is_gamepad_button_released(GLFW_GAMEPAD_BUTTON_Y);
    case KeyCode::L:
      return is_gamepad_button_released(GLFW_GAMEPAD_BUTTON_B);
    case KeyCode::ShiftLeft:
    case KeyCode::ShiftRight:
      return is_gamepad_button_released(GLFW_GAMEPAD_BUTTON_RIGHT_BUMPER);
    default:
      return false;
    }
  }

  bool is_non_escape_controller_input_pressed()
  {
    return is_gamepad_button_pressed(GLFW_GAMEPAD_BUTTON_A)
      || is_gamepad_button_pressed(GLFW_GAMEPAD_BUTTON_B)
      || is_gamepad_button_pressed(GLFW_GAMEPAD_BUTTON_X)
      || is_gamepad_button_pressed(GLFW_GAMEPAD_BUTTON_Y)
      || is_gamepad_button_pressed(GLFW_GAMEPAD_BUTTON_LEFT_BUMPER)
      || is_gamepad_button_pressed(GLFW_GAMEPAD_BUTTON_RIGHT_BUMPER)
      || is_gamepad_button_pressed(GLFW_GAMEPAD_BUTTON_BACK)
      || is_gamepad_button_pressed(GLFW_GAMEPAD_BUTTON_DPAD_LEFT)
      || is_gamepad_button_pressed(GLFW_GAMEPAD_BUTTON_DPAD_RIGHT)
      || is_gamepad_button_pressed(GLFW_GAMEPAD_BUTTON_DPAD_UP)
      || is_gamepad_button_pressed(GLFW_GAMEPAD_BUTTON_DPAD_DOWN)
      || is_gamepad_direction_pressed(GamepadDirection::Left)
      || is_gamepad_direction_pressed(GamepadDirection::Right)
      || is_gamepad_direction_pressed(GamepadDirection::Up)
      || is_gamepad_direction_pressed(GamepadDirection::Down);
  }

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

    any_key_pressed_this_frame = false;

    // Update current mouse button state
    for (int button = 0; button <= GLFW_MOUSE_BUTTON_LAST; ++button)
    {
      currentKeys[button] = (glfwGetMouseButton(window, button) == GLFW_PRESS);
      if (currentKeys[button] && !previousKeys[button])
        any_key_pressed_this_frame = true;
    }

    // Update current keyboard state
    for (int key = 0; key <= GLFW_KEY_LAST; ++key)
    {
      int idx = key + GLFW_MOUSE_BUTTON_LAST + 1;
      currentKeys[idx] = (glfwGetKey(window, key) == GLFW_PRESS);
      if (key != GLFW_KEY_ESCAPE && currentKeys[idx] && !previousKeys[idx])
        any_key_pressed_this_frame = true;
    }

    update_gamepad_state();
    if (is_non_escape_controller_input_pressed())
    {
      any_key_pressed_this_frame = true;
    }


    // Update mouse position
    f64 xpos, ypos;
    glfwGetCursorPos(window, &xpos, &ypos);

    //Log::info("Raw Mouse Position: ({}, {})", xpos, ypos);

    // Convert screen coordinates to viewport-relative virtual coordinates
    mousePosition = Engine::Graphics::screen_to_viewport({ static_cast<f32>(xpos), static_cast<f32>(ypos) });

    //Log::info("Mouse Position: ({}, {})", mousePosition.x, mousePosition.y);

    Engine::Perf::stop("Input");
  }

  bool Input::is_pressed(KeyCode key)
  {
#ifndef GAME_BUILD
    if (ImGuiManager::is_game_input_blocked())
      return false;
#endif
    int idx = key_to_index(key);
  return (currentKeys[idx] && !previousKeys[idx]) || is_controller_mapping_pressed(key);
  }

  bool Input::is_released(KeyCode key)
  {
#ifndef GAME_BUILD
    if (ImGuiManager::is_game_input_blocked())
      return false;
#endif
    int idx = key_to_index(key);
  return (!currentKeys[idx] && previousKeys[idx]) || is_controller_mapping_released(key);
  }

  bool Input::is_held(KeyCode key)
  {
#ifndef GAME_BUILD
    if (ImGuiManager::is_game_input_blocked())
      return false;
#endif
    int idx = key_to_index(key);
  return currentKeys[idx] || is_controller_mapping_held(key);
  }

  bool Input::is_pressed_raw(KeyCode key)
  {
    int idx = key_to_index(key);
    return currentKeys[idx] && !previousKeys[idx];
  }

  bool Input::is_released_raw(KeyCode key)
  {
    int idx = key_to_index(key);
    return !currentKeys[idx] && previousKeys[idx];
  }

  bool Input::is_held_raw(KeyCode key)
  {
    int idx = key_to_index(key);
    return currentKeys[idx];
  }

  bool Input::is_any_key_pressed()
  {
    return any_key_pressed_this_frame;
  }

  glm::vec2 Input::get_mouse_position()
  {
    return mousePosition;
  }

  std::optional<glm::vec2> Input::get_mouse_position_if_in_viewport()
  {
#ifndef GAME_BUILD
    if (ImGuiManager::is_viewport_mode_enabled()) {
      // In viewport mode: only allow input if hovering over the viewport image
      if (!ViewportPanel::is_viewport_hovered())
        return std::nullopt;
    }
    else {
      // In normal mode: block input if any ImGui window is capturing the mouse
      if (ImGui::GetIO().WantCaptureMouse)
        return std::nullopt;
    }
#endif
    return mousePosition;
  }

  glm::vec2 Input::get_mouse_world_position()
  {
    return Engine::Graphics::virtual_to_world(mousePosition);
  }

  std::optional<glm::vec2> Input::get_mouse_world_position_if_in_viewport()
  {
#ifndef GAME_BUILD
    if (ImGuiManager::is_viewport_mode_enabled()) {
      // In viewport mode: only allow input if hovering over the viewport image
      if (!ViewportPanel::is_viewport_hovered())
        return std::nullopt;
    }
    else {
      // In normal mode: block input if any ImGui window is capturing the mouse
      if (ImGui::GetIO().WantCaptureMouse)
        return std::nullopt;
    }
#endif
    return Engine::Graphics::virtual_to_world(mousePosition);
  }

  glm::vec2 Input::get_scroll_offset()
  {
#ifndef GAME_BUILD
    if (ImGuiManager::is_game_input_blocked())
    {
      scrollOffset = { 0.0f, 0.0f };
      return { 0.0f, 0.0f };
    }
#endif
    glm::vec2 result = scrollOffset;
    scrollOffset = { 0.0f, 0.0f }; // Reset after reading
    return result;
  }

  void Input::set_scroll_offset(f64 xoffset, f64 yoffset)
  {
    //Log::info("Scroll Offset Callback: ({}, {})", xoffset, yoffset);
    scrollOffset = { static_cast<f32>(xoffset), static_cast<f32>(yoffset) };
  }
}
