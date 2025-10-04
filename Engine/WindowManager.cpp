//-----------------------------------------------------------------------------------
// file:    WindowManager.cpp
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   This file implements the WindowManager namespace which handles GLFW window
//          creation, configuration, event callbacks, and OpenGL context management
//          with automatic aspect ratio constraints.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#include "WindowManager.h"
#include "Core.h"
#include "BasicEvents.h"
#include <iostream>

namespace Engine {
  namespace WindowManager
  {
    namespace
    {
      // Internal state
      GLFWwindow* window = nullptr;
      s32 width = 0;
      s32 height = 0;
    }

    /* ===================== //
    // Initialization        //
    // ===================== */
    void init(s32 w, s32 h, const std::string& title)
    {
      width = w;
      height = h;

      if (!glfwInit())
      {
        std::cout << "Failed to initialize GLFW" << std::endl;
        return;
      }

      window = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);
      if (!window)
      {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return;
      }

      glfwMakeContextCurrent(window);
      glfwSwapInterval(0);  // Disable VSync

      if (glewInit() != GLEW_OK)
      {
        std::cout << "GLEW Initialization failed." << std::endl;
        return;
      }

      std::cout << "OpenGL Version: " << glGetString(GL_VERSION) << std::endl;

      // Set aspect ratio constraint (16:9)
      glfwSetWindowAspectRatio(window, 16, 9);

      // Set up the resize callback - this handles both window and framebuffer resize
      glfwSetFramebufferSizeCallback(window, framebuffer_resize_callback);

      // Set up file drop callback
      glfwSetDropCallback(window, file_drop_callback);
    }

    void shutdown()
    {
      if (window)
      {
        glfwDestroyWindow(window);
        window = nullptr;
      }
      glfwTerminate();
    }

    /* ===================== //
    // Accessors             //
    // ===================== */
    GLFWwindow* get_window()
    {
      return window;
    }

    s32 get_width()
    {
      return width;
    }

    s32 get_height()
    {
      return height;
    }

    /* ===================== //
    // Callbacks             //
    // ===================== */
    void framebuffer_resize_callback([[maybe_unused]] GLFWwindow* window, s32 w, s32 h)
    {
      // Update the stored dimensions
      width = w;
      height = h;

      // Send resize event to engine (this will update Graphics viewport)
      ECS::World::send_event<WindowResizeEvent>(width, height);
    }

    void file_drop_callback([[maybe_unused]] GLFWwindow* window, s32 count, const char** paths)
    {
      std::vector<std::string> filePaths;
      for (s32 i = 0; i < count; ++i)
      {
        filePaths.push_back(paths[i]);
      }

      ECS::World::send_event<FileDropEvent>(filePaths);
    }
  }
}