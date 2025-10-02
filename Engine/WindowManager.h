//-----------------------------------------------------------------------------------
// file:    WindowManager.h
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   This file defines the WindowManager class which handles GLFW window
//          creation, configuration, event callbacks, and OpenGL context management
//          with automatic aspect ratio constraints.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once

#include "Precompiled.h"
#include <string>
#include <functional>
#include "TypeDef.h"

class WindowManager
{
private:
  GLFWwindow* window;
  s32 width = 0;
  s32 height = 0;
  std::string title;

  /* ===================== //
  // Static Callbacks      //
  // ===================== */
  /*=================================================================================
  Function Brief:
  Static callback function for GLFW framebuffer resize events. Updates internal
  dimensions and dispatches WindowResizeEvent to the engine.

  Parameters:
  window - The GLFW window that was resized
  width - New framebuffer width in pixels
  height - New framebuffer height in pixels
  ==================================================================================*/
  static void framebufferSizeCallback(GLFWwindow* window, s32 width, s32 height);

  /*=================================================================================
  Function Brief:
  Static callback function for GLFW file drop events. Dispatches FileDropEvent
  to the engine with the paths of dropped files.

  Parameters:
  window - The GLFW window that received the drop
  count - Number of files dropped
  paths - Array of file path strings
  ==================================================================================*/
  static void file_drop_callback(GLFWwindow* window, s32 count, const char** paths);

public:
  /*=================================================================================
  Function Brief:
  Constructs a new WindowManager instance with null window pointer.
  ==================================================================================*/
  WindowManager();

  /*=================================================================================
  Function Brief:
  Initializes GLFW, creates the window, sets up OpenGL context, and registers
  event callbacks. Configures 16:9 aspect ratio constraint and disables VSync.

  Where to call it:
  Call this after set_params() to create and configure the game window.
  ==================================================================================*/
  void window_init();

  /*=================================================================================
  Function Brief:
  Sets the window parameters including dimensions and title.

  Parameters:
  width - Window width in pixels
  height - Window height in pixels
  win_name - Window title string

  Where to call it:
  Call this before window_init() to configure window properties.
  ==================================================================================*/
  void set_params(s32 width, s32 height, std::string win_name);

  /*=================================================================================
  Function Brief:
  Destructor that destroys the GLFW window and terminates GLFW.
  ==================================================================================*/
  ~WindowManager();

  /* ===================== //
  // Accessors             //
  // ===================== */
  GLFWwindow* getWindow() const { return window; }
  s32 get_width() const { return width; }
  s32 get_height() const { return height; }

};

