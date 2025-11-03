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
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once

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

} // namespace ViewportPanel
