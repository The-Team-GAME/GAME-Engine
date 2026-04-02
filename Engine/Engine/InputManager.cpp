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
#include <cstddef>
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
  constexpr std::size_t INPUT_STATE_COUNT = static_cast<std::size_t>(GLFW_MOUSE_BUTTON_LAST + GLFW_KEY_LAST + 2);
  using InputStateArray = std::array<bool, INPUT_STATE_COUNT>;

  constexpr float GAMEPAD_AXIS_PRESS_THRESHOLD = 0.45f;

  /* =================== //
  // Input state arrays  //
  // =================== */
  InputStateArray currentKeys{};
  InputStateArray previousKeys{};
  InputStateArray currentVirtualKeys{};
  InputStateArray previousVirtualKeys{};
  bool any_key_pressed_this_frame = false;
  bool last_input_was_mouse = true;
  bool has_mouse_position = false;
  bool mouse_scrolled_this_frame = false;
  f64 raw_mouse_x = 0.0;
  f64 raw_mouse_y = 0.0;

  /* =================== //
  // Mouse state         //
  // =================== */
  glm::vec2 mousePosition;
  glm::vec2 scrollOffset;

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

  inline void set_virtual_key(InputStateArray& states, Engine::KeyCode key, bool value)
  {
    states[key_to_index(key)] = value;
  }

  inline bool get_combined_state(const InputStateArray& raw_states, const InputStateArray& virtual_states, Engine::KeyCode key)
  {
    const int idx = key_to_index(key);
    return raw_states[idx] || virtual_states[idx];
  }

  struct GamepadInputState
  {
    bool up = false;
    bool down = false;
    bool left = false;
    bool right = false;
    bool jump = false;
    bool attack = false;
    bool downsmash = false;
    bool uppercut = false;
    bool dash = false;
    bool upgrades = false;
    bool pause = false;
  };

  GamepadInputState poll_gamepad_input()
  {
#if GLFW_HAS_GAMEPAD_API && !defined(EMSCRIPTEN_USE_EMBEDDED_GLFW3)
    for (int joystick_id = GLFW_JOYSTICK_1; joystick_id <= GLFW_JOYSTICK_LAST; ++joystick_id)
    {
      if (!glfwJoystickPresent(joystick_id) || !glfwJoystickIsGamepad(joystick_id))
      {
        continue;
      }

      GLFWgamepadstate gamepad_state{};
      if (!glfwGetGamepadState(joystick_id, &gamepad_state))
      {
        continue;
      }

      GamepadInputState state{};
      state.up = gamepad_state.buttons[GLFW_GAMEPAD_BUTTON_DPAD_UP] == GLFW_PRESS
        || gamepad_state.axes[GLFW_GAMEPAD_AXIS_LEFT_Y] <= -GAMEPAD_AXIS_PRESS_THRESHOLD;
      state.down = gamepad_state.buttons[GLFW_GAMEPAD_BUTTON_DPAD_DOWN] == GLFW_PRESS
        || gamepad_state.axes[GLFW_GAMEPAD_AXIS_LEFT_Y] >= GAMEPAD_AXIS_PRESS_THRESHOLD;
      state.left = gamepad_state.buttons[GLFW_GAMEPAD_BUTTON_DPAD_LEFT] == GLFW_PRESS
        || gamepad_state.axes[GLFW_GAMEPAD_AXIS_LEFT_X] <= -GAMEPAD_AXIS_PRESS_THRESHOLD;
      state.right = gamepad_state.buttons[GLFW_GAMEPAD_BUTTON_DPAD_RIGHT] == GLFW_PRESS
        || gamepad_state.axes[GLFW_GAMEPAD_AXIS_LEFT_X] >= GAMEPAD_AXIS_PRESS_THRESHOLD;
      state.jump = gamepad_state.buttons[GLFW_GAMEPAD_BUTTON_A] == GLFW_PRESS;
      state.downsmash = gamepad_state.buttons[GLFW_GAMEPAD_BUTTON_B] == GLFW_PRESS;
      state.attack = gamepad_state.buttons[GLFW_GAMEPAD_BUTTON_X] == GLFW_PRESS;
      state.uppercut = gamepad_state.buttons[GLFW_GAMEPAD_BUTTON_Y] == GLFW_PRESS;
      state.upgrades = gamepad_state.buttons[GLFW_GAMEPAD_BUTTON_LEFT_BUMPER] == GLFW_PRESS;
      state.dash = gamepad_state.buttons[GLFW_GAMEPAD_BUTTON_RIGHT_BUMPER] == GLFW_PRESS;
      state.pause = gamepad_state.buttons[GLFW_GAMEPAD_BUTTON_BACK] == GLFW_PRESS
        || gamepad_state.buttons[GLFW_GAMEPAD_BUTTON_START] == GLFW_PRESS;
      return state;
    }
#endif

    for (int joystick_id = GLFW_JOYSTICK_1; joystick_id <= GLFW_JOYSTICK_LAST; ++joystick_id)
    {
      if (!glfwJoystickPresent(joystick_id))
      {
        continue;
      }

      int axes_count = 0;
      int buttons_count = 0;
      const float* axes = glfwGetJoystickAxes(joystick_id, &axes_count);
      const unsigned char* buttons = glfwGetJoystickButtons(joystick_id, &buttons_count);
      if (axes_count <= 0 && buttons_count <= 0)
      {
        continue;
      }

      GamepadInputState state{};
      state.up = (buttons_count > 10 && buttons[10] == GLFW_PRESS)
        || (axes_count > 1 && axes[1] <= -GAMEPAD_AXIS_PRESS_THRESHOLD);
      state.down = (buttons_count > 12 && buttons[12] == GLFW_PRESS)
        || (axes_count > 1 && axes[1] >= GAMEPAD_AXIS_PRESS_THRESHOLD);
      state.left = (buttons_count > 13 && buttons[13] == GLFW_PRESS)
        || (axes_count > 0 && axes[0] <= -GAMEPAD_AXIS_PRESS_THRESHOLD);
      state.right = (buttons_count > 11 && buttons[11] == GLFW_PRESS)
        || (axes_count > 0 && axes[0] >= GAMEPAD_AXIS_PRESS_THRESHOLD);
      state.jump = buttons_count > 0 && buttons[0] == GLFW_PRESS;
      state.downsmash = buttons_count > 1 && buttons[1] == GLFW_PRESS;
      state.attack = buttons_count > 2 && buttons[2] == GLFW_PRESS;
      state.uppercut = buttons_count > 3 && buttons[3] == GLFW_PRESS;
      state.upgrades = buttons_count > 4 && buttons[4] == GLFW_PRESS;
      state.dash = buttons_count > 5 && buttons[5] == GLFW_PRESS;
      state.pause = (buttons_count > 6 && buttons[6] == GLFW_PRESS)
        || (buttons_count > 7 && buttons[7] == GLFW_PRESS);
      return state;
    }

    return {};
  }

  void update_virtual_gamepad_keys()
  {
    currentVirtualKeys.fill(false);

    const GamepadInputState gamepad_state = poll_gamepad_input();

    set_virtual_key(currentVirtualKeys, Engine::KeyCode::W, gamepad_state.up);
    set_virtual_key(currentVirtualKeys, Engine::KeyCode::Up, gamepad_state.up);
    set_virtual_key(currentVirtualKeys, Engine::KeyCode::S, gamepad_state.down);
    set_virtual_key(currentVirtualKeys, Engine::KeyCode::Down, gamepad_state.down);
    set_virtual_key(currentVirtualKeys, Engine::KeyCode::A, gamepad_state.left);
    set_virtual_key(currentVirtualKeys, Engine::KeyCode::Left, gamepad_state.left);
    set_virtual_key(currentVirtualKeys, Engine::KeyCode::D, gamepad_state.right);
    set_virtual_key(currentVirtualKeys, Engine::KeyCode::Right, gamepad_state.right);
    set_virtual_key(currentVirtualKeys, Engine::KeyCode::Space, gamepad_state.jump);
    set_virtual_key(currentVirtualKeys, Engine::KeyCode::J, gamepad_state.attack);
    set_virtual_key(currentVirtualKeys, Engine::KeyCode::L, gamepad_state.downsmash);
    set_virtual_key(currentVirtualKeys, Engine::KeyCode::K, gamepad_state.uppercut);
    set_virtual_key(currentVirtualKeys, Engine::KeyCode::ShiftLeft, gamepad_state.dash);
    set_virtual_key(currentVirtualKeys, Engine::KeyCode::Tab, gamepad_state.upgrades);
    set_virtual_key(currentVirtualKeys, Engine::KeyCode::Escape, gamepad_state.pause);
  }

  void update_any_key_pressed_state()
  {
    any_key_pressed_this_frame = false;

    const int escape_index = key_to_index(Engine::KeyCode::Escape);

    for (int idx = 0; idx < static_cast<int>(currentKeys.size()); ++idx)
    {
      if (idx == escape_index)
      {
        continue;
      }

      const bool current_state = currentKeys[idx] || currentVirtualKeys[idx];
      const bool previous_state = previousKeys[idx] || previousVirtualKeys[idx];
      if (current_state && !previous_state)
      {
        any_key_pressed_this_frame = true;
        return;
      }
    }
  }

  bool has_mouse_button_activity()
  {
    for (int button = 0; button <= GLFW_MOUSE_BUTTON_LAST; ++button)
    {
      if (currentKeys[button] != previousKeys[button])
      {
        return true;
      }
    }

    return false;
  }

  bool has_keyboard_activity()
  {
    for (int key = 0; key <= GLFW_KEY_LAST; ++key)
    {
      const int idx = key + GLFW_MOUSE_BUTTON_LAST + 1;
      if (currentKeys[idx] != previousKeys[idx])
      {
        return true;
      }
    }

    return false;
  }

  bool has_virtual_key_activity()
  {
    for (int idx = 0; idx < static_cast<int>(currentVirtualKeys.size()); ++idx)
    {
      if (currentVirtualKeys[idx] != previousVirtualKeys[idx])
      {
        return true;
      }
    }

    return false;
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
    previousVirtualKeys = currentVirtualKeys;

    // Update current mouse button state
    for (int button = 0; button <= GLFW_MOUSE_BUTTON_LAST; ++button)
    {
      currentKeys[button] = (glfwGetMouseButton(window, button) == GLFW_PRESS);
    }

    // Update current keyboard state
    for (int key = 0; key <= GLFW_KEY_LAST; ++key)
    {
      int idx = key + GLFW_MOUSE_BUTTON_LAST + 1;
      currentKeys[idx] = (glfwGetKey(window, key) == GLFW_PRESS);
    }

    update_virtual_gamepad_keys();
    update_any_key_pressed_state();


    // Update mouse position
    f64 xpos, ypos;
    glfwGetCursorPos(window, &xpos, &ypos);

    const bool mouse_moved = has_mouse_position && (xpos != raw_mouse_x || ypos != raw_mouse_y);
    raw_mouse_x = xpos;
    raw_mouse_y = ypos;
    has_mouse_position = true;

    const bool mouse_activity = mouse_moved || has_mouse_button_activity() || mouse_scrolled_this_frame;
    const bool non_mouse_activity = has_keyboard_activity() || has_virtual_key_activity();
    if (mouse_activity)
    {
      last_input_was_mouse = true;
    }
    else if (non_mouse_activity)
    {
      last_input_was_mouse = false;
    }
    mouse_scrolled_this_frame = false;

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
    return is_pressed_raw(key);
  }

  bool Input::is_released(KeyCode key)
  {
#ifndef GAME_BUILD
    if (ImGuiManager::is_game_input_blocked())
      return false;
#endif
    return is_released_raw(key);
  }

  bool Input::is_held(KeyCode key)
  {
#ifndef GAME_BUILD
    if (ImGuiManager::is_game_input_blocked())
      return false;
#endif
    return is_held_raw(key);
  }

  bool Input::is_pressed_raw(KeyCode key)
  {
    return get_combined_state(currentKeys, currentVirtualKeys, key)
      && !get_combined_state(previousKeys, previousVirtualKeys, key);
  }

  bool Input::is_released_raw(KeyCode key)
  {
    return !get_combined_state(currentKeys, currentVirtualKeys, key)
      && get_combined_state(previousKeys, previousVirtualKeys, key);
  }

  bool Input::is_held_raw(KeyCode key)
  {
    return get_combined_state(currentKeys, currentVirtualKeys, key);
  }

  bool Input::is_any_key_pressed()
  {
    return any_key_pressed_this_frame;
  }

  bool Input::was_last_input_mouse()
  {
    return last_input_was_mouse;
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
    mouse_scrolled_this_frame = true;
  }
}
