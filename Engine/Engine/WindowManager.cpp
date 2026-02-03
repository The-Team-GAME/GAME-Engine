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
#include "Events/WindowEvents.h"
#include "Logger.h"
#include "ECS/ECS.h"
#include "Time.h"
#include <sstream>
#include <iomanip>
#include "InputManager.h"
#include "ProjectSaveManager.h"
#include "LevelManager.h"

namespace Engine {
  namespace WindowManager
  {
    namespace
    {
      // Internal state
      GLFWwindow* window = nullptr;
      s32 width = 0;
      s32 height = 0;
      std::string base_title = "";

      // FPS tracking variables
      f32 display_fps = 0.0f;
      f32 update_timer = 0.0f;
      s32 frame_count = 0;
      const f32 update_interval = 0.25f;
    }

    /* ===================== //
    // Initialization        //
    // ===================== */
    void init(s32 w, s32 h, const std::string& title)
    {
      width = w;
      height = h;
      base_title = title;

      if (!glfwInit())
      {
        Log::error("Failed to initialize GLFW");
        return;
      }

      window = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);
      if (!window)
      {
        Log::error("Failed to create GLFW window");
        glfwTerminate();
        return;
      }

      glfwMakeContextCurrent(window);
      glfwSwapInterval(0);  // Disable VSync

      if (glewInit() != GLEW_OK)
      {
        Log::error("GLEW Initialization failed.");
        return;
      }

      Log::info("OpenGL Version: {}", reinterpret_cast<const char*>(glGetString(GL_VERSION)));

      // Set aspect ratio constraint (16:9)
      glfwSetWindowAspectRatio(window, 16, 9);

      // Set up the resize callback - this handles both window and framebuffer resize
      glfwSetFramebufferSizeCallback(window, framebuffer_resize_callback);

      // Set up file drop callback
      glfwSetDropCallback(window, file_drop_callback);

      // Set up scroll callback
      glfwSetScrollCallback(window, scroll_callback);

      // Set up window close callback
      glfwSetWindowCloseCallback(window, window_close_callback);
    }

    void exit()
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
    // Title Update          //
    // ===================== */
    void update_title()
    {
      if (!window) return;

      update_timer += Engine::Time::get_real_dt();
      frame_count++;

      if (update_timer >= update_interval)
      {
        // Calculate average FPS over the interval (same as PerformanceViewerPanel)
        display_fps = frame_count / update_timer;

        bool is_unsaved = Engine::ProjectSaveManager::is_any_dirty();

        // Update window title
        std::ostringstream title_stream;
        title_stream << (is_unsaved ? "[*] " : "") << base_title << " - " << Engine::LevelManager::get_current_level_path() << " - FPS: " << std::fixed << std::setprecision(1) << display_fps;
        glfwSetWindowTitle(window, title_stream.str().c_str());

        update_timer = 0.0f;
        frame_count = 0;
      }
    }

    /* ===================== //
    // Window Mode           //
    // ===================== */
    void set_fullscreen(bool fullscreen)
    {
      if (!window) return;

      if (fullscreen)
      {
        // Get primary monitor and its video mode
        GLFWmonitor* monitor = glfwGetPrimaryMonitor();
        const GLFWvidmode* mode = glfwGetVideoMode(monitor);

        // Switch to fullscreen
        glfwSetWindowMonitor(window, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
      }
      else
      {
        // Switch to windowed mode
        // Center the window on the screen
        GLFWmonitor* monitor = glfwGetPrimaryMonitor();
        const GLFWvidmode* mode = glfwGetVideoMode(monitor);

        // Use default size or stored size
        int w = 1600;
        int h = 900;

        int x = (mode->width - w) / 2;
        int y = (mode->height - h) / 2;

        glfwSetWindowMonitor(window, nullptr, x, y, w, h, 0);
      }
    }

    /* ===================== //
    // Callbacks             //
    // ===================== */
    void framebuffer_resize_callback([[maybe_unused]] GLFWwindow* win, s32 w, s32 h)
    {
      // Update the stored dimensions
      width = w;
      height = h;

      // Send resize event to engine (this will update Graphics viewport)
      Engine::EventManager::send_event<Engine::Events::WindowResizeEvent>(width, height);
    }

    void file_drop_callback([[maybe_unused]] GLFWwindow* win, s32 count, const char** paths)
    {
      std::vector<std::string> filePaths;
      for (s32 i = 0; i < count; ++i)
      {
        filePaths.push_back(paths[i]);
      }

      Engine::EventManager::send_event<Engine::Events::FileDropEvent>(filePaths);
    }

    void scroll_callback([[maybe_unused]] GLFWwindow* win, f64 xoffset, f64 yoffset)
    {
      Engine::Input::set_scroll_offset(xoffset, yoffset);
    }

    void window_close_callback([[maybe_unused]] GLFWwindow* win)
    {
#ifndef GAME_BUILD
      // Cancel the close - we'll handle it through ImGui popup
      glfwSetWindowShouldClose(win, GLFW_FALSE);

      // Set flag to show the exit confirmation popup
      Engine::EventManager::send_event<Engine::Events::WindowCloseRequestedEvent>();
#endif
    }
  }
}