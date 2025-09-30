#pragma once

#include "Precompiled.h"
#include <string>
#include <functional>
#include "TypeDef.h"

class WindowManager
{
private:
  GLFWwindow* window;
  s32 width;
  s32 height;
  std::string title;

  // Static callback function for GLFW
  static void framebufferSizeCallback(GLFWwindow* window, s32 width, s32 height);

public:
  WindowManager();
  void window_init();
  void set_params(s32 width, s32 height, std::string win_name);
  ~WindowManager();

  GLFWwindow* getWindow() const { return window; }
  s32 getWidth() const { return width; }
  s32 getHeight() const { return height; }

};

