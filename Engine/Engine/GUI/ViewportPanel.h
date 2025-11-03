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
  Converts mouse position from screen coordinates to viewport virtual coordinates.
  Takes into account the viewport's position, size, and aspect ratio.

  Parameters:
  screen_x - Mouse X position in screen coordinates (pixels)
  screen_y - Mouse Y position in screen coordinates (pixels)

  Returns:
  Math::Vec2D - Mouse position in virtual coordinates (engine coordinate space)

  Where to call it:
  Call this when you need to convert mouse input to game coordinates while using
  the viewport panel.
  ==================================================================================*/
  Math::Vec2D screen_to_viewport_virtual(float screen_x, float screen_y);

  /*=================================================================================
  Function Brief:
  Gets the mouse position in viewport virtual coordinates if the mouse is over
  the viewport. Returns the position adjusted for the viewport's bounds.

  Returns:
  Math::Vec2D - Mouse position in virtual coordinates, or {0,0} if not hovering

  Where to call it:
  Call this to get properly adjusted mouse coordinates for game input when using
  the viewport panel.
  ==================================================================================*/
  Math::Vec2D get_viewport_mouse_position();

  /*=================================================================================
  Function Brief:
  Gets the mouse position in world coordinates (accounting for camera) if the
  mouse is over the viewport.

  Returns:
  Math::Vec2D - Mouse position in world coordinates, or {0,0} if not hovering

  Where to call it:
  Call this to get world-space mouse coordinates for game input when using
  the viewport panel.
  ==================================================================================*/
  Math::Vec2D get_viewport_mouse_world_position();

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

} // namespace ViewportPanel
