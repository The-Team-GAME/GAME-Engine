#include "WindowManager.h"
#include "Core.h"
#include "BasicEvents.h"
#include <iostream>

namespace Engine
{
  WindowManager::WindowManager(int w, int h, const std::string& t)
    : width(w), height(h), title(t), window(nullptr)
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

    // Set aspect ratio constraint (16:9)
    glfwSetWindowAspectRatio(window, 16, 9);

    // Set up the resize callback - this handles both window and framebuffer resize
    glfwSetWindowUserPointer(window, this);
    glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);
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

  void WindowManager::framebufferSizeCallback(GLFWwindow* window, int width, int height) {
    // Get the WindowManager instance from the user pointer
    WindowManager* windowManager = static_cast<WindowManager*>(glfwGetWindowUserPointer(window));

    if (windowManager) {
      // Update the stored dimensions
      windowManager->width = width;
      windowManager->height = height;

      // Send resize event to engine (this will update Graphics viewport)
      Engine::WORLD.sendEvent<WindowResizeEvent>(width, height);
    }
  }
}