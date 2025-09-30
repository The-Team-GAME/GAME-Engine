#include "WindowManager.h"
#include "Core.h"
#include "BasicEvents.h"
#include <iostream>


WindowManager::WindowManager()
  : window(nullptr) 
{
  // Empty by design
}

void WindowManager::set_param(s32 w, s32 h, std::string win_name)
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

void WindowManager::framebufferSizeCallback(GLFWwindow* window, s32 width, s32 height) {
  // Get the WindowManager instance from the user pos32er
  WindowManager* windowManager = static_cast<WindowManager*>(glfwGetWindowUserPointer(window));

  if (windowManager) {
    // Update the stored dimensions
    windowManager->width = width;
    windowManager->height = height;

    // Send resize event to engine (this will update Graphics viewport)
    Engine::WORLD.sendEvent<WindowResizeEvent>(width, height);
  }
}
