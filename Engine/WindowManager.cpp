#include "WindowManager.h"
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
    glfwSwapInterval(0);  // Disable VSync; adjust as needed

    if (glewInit() != GLEW_OK)
    {
      std::cout << "GLEW Initialization failed." << std::endl;
      return;
    }

    std::cout << "OpenGL Version: " << glGetString(GL_VERSION) << std::endl;
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
}