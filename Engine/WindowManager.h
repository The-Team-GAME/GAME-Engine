#pragma once

#include "Precompiled.h"
#include <string>
#include <functional>

namespace Engine
{
  class WindowManager
  {
  private:
    GLFWwindow* window;
    int width;
    int height;
    std::string title;

    // Static callback function for GLFW
    static void framebufferSizeCallback(GLFWwindow* window, int width, int height);
    static void fileDropCallback(GLFWwindow* window, int count, const char** paths);

  public:
    WindowManager(int w, int h, const std::string& t);
    ~WindowManager();

    GLFWwindow* getWindow() const { return window; }
    int getWidth() const { return width; }
    int getHeight() const { return height; }

  };
}
