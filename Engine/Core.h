#pragma once

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include "ECS/ECS.h"

namespace Engine
{
  class Core
  {
  private:
    bool isRunning;
    float lastTime;
    GLFWwindow *window;
    float dt;

  public:
    Core();
    ~Core();

    int init();
    void run();
    void exit();
    GLFWwindow *getWindow() const;
  };

  // Global pointer to the instance of the core
  extern Core *CORE;
  extern ECS::World *CORE_ECS;
}