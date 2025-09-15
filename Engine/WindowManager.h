#pragma once

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <string>

namespace Engine
{
  class WindowManager
  {
  private:
    GLFWwindow* window;
    int width;
    int height;
    std::string title;

  public:
    WindowManager(int w, int h, const std::string& t);
    ~WindowManager();

    GLFWwindow* getWindow() const { return window; }
    int getWidth() const { return width; }
    int getHeight() const { return height; }

    // Add more methods as needed, e.g., for resizing or fullscreen
  };
}
