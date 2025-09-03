#pragma once

#include <GL/glew.h>
#include <GLFW/glfw3.h>

namespace Engine
{
  class Core
  {
  private:
    bool isRunning;
    unsigned lastTime;
    GLFWwindow *window;

  public:
    Core();
    ~Core();

    int init();
    void gameLoop();
    void exit();
  };

  // Global pointer to the instance of the core
  extern Core *CORE;
}