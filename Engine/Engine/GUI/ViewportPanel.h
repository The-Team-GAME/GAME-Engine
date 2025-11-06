//-----------------------------------------------------------------------------------
// file:    ViewportPanel.h
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   This file defines the ViewportPanel ImGui interface for displaying the
//          game viewport inside an ImGui window. Renders the game scene to a
//          framebuffer texture and displays it in a resizable ImGui window.
//
//          INPUT HANDLING:
//          The InputManager automatically uses viewport coordinates when the mouse
//          is hovering over the viewport. Use ViewportPanel::should_capture_input()
//          to check if the viewport is active before processing game input events.
//
//          Example usage:
//            if (ViewportPanel::should_capture_input()) {
//              // Process game input here
//              if (Input::is_pressed(KeyCode::MouseLeft)) {
//                // Mouse click in game world
//                auto world_pos = Input::get_mouse_world_position();
//              }
//            }
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once

#include "../Math/Vector2D.h"

namespace ViewportPanel {

  /*=================================================================================
  Function Brief:
  Initializes the framebuffer and render texture for the viewport panel.
  Must be called once during engine initialization.

  Where to call it:
  Called during engine initialization after OpenGL context is created.
  ==================================================================================*/
  void init();

  /*=================================================================================
  Function Brief:
  Cleans up framebuffer and texture resources.

  Where to call it:
  Called during engine shutdown.
  ==================================================================================*/
  void exit();

  /*=================================================================================
  Function Brief:
  Binds the framebuffer for rendering. All subsequent draw calls will render to
  the framebuffer texture instead of the screen.

  Where to call it:
  Called before rendering the game scene (before StateManager draw and ECS draw).
  ==================================================================================*/
  void begin_render();

  /*=================================================================================
  Function Brief:
  Unbinds the framebuffer, restoring rendering to the default framebuffer (screen).

  Where to call it:
  Called after all game rendering is complete (after ECS draw).
  ==================================================================================*/
  void end_render();

  /*=================================================================================
  Function Brief:
  Renders the ImGui panel displaying the viewport texture.

  Where to call it:
  Called during the ImGui phase of the main game loop.
  ==================================================================================*/
  void draw_panel();

  /*=================================================================================
  Function Brief:
  Returns true if the mouse is currently hovering over the viewport window.

  Returns:
  bool - True if viewport is hovered, false otherwise

  Where to call it:
  Can be called to check if input should be handled by the viewport.
  ==================================================================================*/
  bool is_viewport_hovered();

  /*=================================================================================
  Function Brief:
  Returns true if the viewport window is currently focused.

  Returns:
  bool - True if viewport is focused, false otherwise

  Where to call it:
  Can be called to check if input should be handled by the viewport.
  ==================================================================================*/
  bool is_viewport_focused();

  /*=================================================================================
  Function Brief:
  Returns whether game input should be processed. This is true when the viewport
  is both hovered and focused, indicating the user is interacting with the game.

  Returns:
  bool - True if game input should be processed, false otherwise

  Where to call it:
  Call this before processing game input to check if the viewport is active.
  Useful for conditionally handling clicks, keyboard input, etc.
  ==================================================================================*/
  bool should_capture_input();

  /*=================================================================================
  Function Brief:
  Gets the viewport bounds in screen coordinates (pixels).

  Parameters:
  min_x - Output: Left edge of viewport in screen space
  min_y - Output: Top edge of viewport in screen space
  max_x - Output: Right edge of viewport in screen space
  max_y - Output: Bottom edge of viewport in screen space

  Where to call it:
  Call this to get the actual pixel bounds of the viewport panel for coordinate
  conversion and input handling.
  ==================================================================================*/
  void get_viewport_bounds(float& min_x, float& min_y, float& max_x, float& max_y);

} // namespace ViewportPanel
