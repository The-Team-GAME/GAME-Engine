//-----------------------------------------------------------------------------------
// file:    InputManager.h
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   This file defines the InputManager which provides a static interface for
//          handling keyboard and mouse input. Supports frame-based input detection
//          (pressed, released, held) and mouse position tracking in virtual coords.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once
#include <glm/glm.hpp>
#include "TypeDef.h"
#include "KeyCodes.h"
#include "EngineExport.h"
#include <optional>

namespace Engine
{
  /**
   * @class Input
   * @brief A static class for handling keyboard and mouse input.
   *
   * This class provides methods to check the state of keyboard keys and mouse buttons.
   * The Update() method must be called once per frame before any other methods are used.
   */
  class ENGINE_API Input
  {
  public:
    /**
     * @brief Updates the internal state of the input manager.
     * This should be called once at the beginning of each frame.
     */
    static void update();

    // Keyboard input
    /**
     * @brief Checks if a key was just pressed in the current frame.
     * @param key The key code to check.
     * @return true if the key was pressed in this frame, false otherwise.
     */
    static bool is_pressed(KeyCode key);

    /**
     * @brief Checks if a key was just released in the current frame.
     * @param key The key code to check.
     * @return true if the key was released in this frame, false otherwise.
     */
    static bool is_released(KeyCode key);

    /**
     * @brief Checks if a key is currently being held down.
     * @param key The key code to check.
     * @return true if the key is held down, false otherwise.
     */
    static bool is_held(KeyCode key);

    /**
     * @brief Raw input methods that bypass ImGui focus-blocking.
     * Use these for editor-wide hotkeys (Ctrl+S, F1-F5, Ctrl+Z, etc.)
     * that should work regardless of which ImGui panel is focused.
     * Do NOT use these for gameplay input — use the regular methods instead.
     */
    static bool is_pressed_raw(KeyCode key);
    static bool is_released_raw(KeyCode key);
    static bool is_held_raw(KeyCode key);

    /**
     * @brief Gets the current position of the mouse cursor in window coordinates.
     * @return glm::vec2 A vector containing the x and y position of the mouse.
     */
    static glm::vec2 get_mouse_position();

    static std::optional<glm::vec2> get_mouse_position_if_in_viewport();

    /**
     * @brief Gets the current position of the mouse cursor in world coordinates.
     * This accounts for camera position, zoom, and rotation.
     * @return glm::vec2 A vector containing the x and y position of the mouse in world space.
     */
    static glm::vec2 get_mouse_world_position();

    static std::optional<glm::vec2> get_mouse_world_position_if_in_viewport();

    /**
     * @brief Gets the scroll offset for the current frame.
     * @return glm::vec2 A vector containing the x and y scroll offset (y is usually used for vertical scrolling).
     */
    static glm::vec2 get_scroll_offset();

    /**
     * @brief Internal callback function to update scroll offset.
     * This is called by the GLFW scroll callback.
     * @param xoffset The horizontal scroll offset.
     * @param yoffset The vertical scroll offset.
     */
    static void set_scroll_offset(f64 xoffset, f64 yoffset);
  };
}