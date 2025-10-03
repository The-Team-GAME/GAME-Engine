//-----------------------------------------------------------------------------------
// file:    WindowManager.cpp
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   This file implements the WindowManager class which handles GLFW window
//          creation, configuration, event callbacks, and OpenGL context management
//          with automatic aspect ratio constraints.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#include "WindowManager.h"
#include "Core.h"
#include "BasicEvents.h"
#include <iostream>

/* ===================== //
// Constructor/Destructor//
// ===================== */
WindowManager::WindowManager()
  : window(nullptr)
{
  // Empty by design
}

/* ===================== //
// Initialization        //
// ===================== */
void WindowManager::set_params(s32 w, s32 h, std::string win_name)
{
  width = w;
  height = h;
  title = win_name;
}

void WindowManager::window_init()
{
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

  // Set aspect ratio constras32 (16:9)
  glfwSetWindowAspectRatio(window, 16, 9);

  // Set up the resize callback - this handles both window and framebuffer resize
  glfwSetWindowUserPointer(window, this);
  glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);

  // Set up file drop callback
  glfwSetDropCallback(window, file_drop_callback);
}

WindowManager::~WindowManager()
{
  if (window)
  {
    glfwDestroyWindow(window);
    window = nullptr;
  }
  glfwTerminate();
}

/* ===================== //
// Callbacks             //
// ===================== */
void WindowManager::framebufferSizeCallback(GLFWwindow* window, s32 width, s32 height) {
  // Get the WindowManager instance from the user pos32er
  WindowManager* windowManager = static_cast<WindowManager*>(glfwGetWindowUserPointer(window));

  if (windowManager) {
    // Update the stored dimensions
    windowManager->width = width;
    windowManager->height = height;

    // Send resize event to engine (this will update Graphics viewport)
    Engine::WORLD.send_event<WindowResizeEvent>(width, height);
  }
}

void WindowManager::file_drop_callback([[maybe_unused]] GLFWwindow* window, s32 count, const char** paths)
{

  std::vector<std::string> filePaths;
  for (s32 i = 0; i < count; ++i)
  {
    filePaths.push_back(paths[i]);
  }

  Engine::WORLD.send_event<FileDropEvent>(filePaths);
}
