//-----------------------------------------------------------------------------------
// file:    WindowManager.h
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   This file defines the WindowManager namespace which handles GLFW window
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

namespace Engine {
  namespace WindowManager
  {
    /*=================================================================================
    Function Brief:
    Initializes GLFW, creates the window, sets up OpenGL context, and registers
    event callbacks. Configures 16:9 aspect ratio constraint and disables VSync.

    Parameters:
    width - Window width in pixels
    height - Window height in pixels
    title - Window title string

    Where to call it:
    Call this to create and configure the game window.
    ==================================================================================*/
    void init(s32 width, s32 height, const std::string& title);

    /*=================================================================================
    Function Brief:
    Destroys the GLFW window and terminates GLFW.

    Where to call it:
    Call this during engine shutdown to clean up window resources.
    ==================================================================================*/
    void shutdown();

    /* ===================== //
    // Accessors             //
    // ===================== */
    GLFWwindow* get_window();
    s32 get_width();
    s32 get_height();

    /* ===================== //
    // Internal Callbacks    //
    // ===================== */
    void framebuffer_resize_callback(GLFWwindow* window, s32 width, s32 height);
    void file_drop_callback(GLFWwindow* window, s32 count, const char** paths);
  }
}