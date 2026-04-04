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
//          The InputManager automatically allows input when hovering over the
//          viewport image, even though it's technically an ImGui window. This means
//          you can interact with game entities in the viewport without other ImGui
//          panels interfering. The viewport is treated as a special exception to
//          normal ImGui input blocking.
//
//          Example usage:
//            auto world_pos_opt = Input::get_mouse_world_position_if_in_viewport();
//            if (world_pos_opt.has_value()) {
//              // Mouse is over viewport - interact with game world
//              auto world_pos = world_pos_opt.value();
//            }
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once

#include "../Math/Vector2D.h"
#include "../Math/Vector4D.h"

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
  Gets the viewport bounds in screen coordinates (pixels).

  Returns:
  Math::Vec4D - Viewport bounds (x: min_x, y: min_y, z: max_x, w: max_y)

  Where to call it:
  Call this to get the actual pixel bounds of the viewport panel for coordinate
  conversion and input handling.
  ==================================================================================*/
  Math::Vec4D get_viewport_bounds();

  /*=================================================================================
  Function Brief:
  Checks if the mouse cursor is hovering over the viewport image (not just the window).

  Returns:
  bool - True if hovering over the viewport image

  Where to call it:
  Used internally by InputManager to determine if viewport should capture input.
  ==================================================================================*/
  bool is_viewport_hovered();

  /*=================================================================================
  Function Brief:
  Checks if the viewport window is currently focused.

  Returns:
  bool - True if the viewport window has focus

  Where to call it:
  Used by ImGuiManager to determine if game input should be blocked.
  ==================================================================================*/
  bool is_viewport_focused();

} // namespace ViewportPanel

