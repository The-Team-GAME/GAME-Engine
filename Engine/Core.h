#pragma once

#include "Graphics.h"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include "ECS/ECS.h"
#include "ObjectFactory.h"

namespace Engine
{
  class Core
  {
  private:
    bool isRunning;
    GLFWwindow* window;

  public:
    Core();
    ~Core();

    int init();
    void run();
    void exit();

    float getDt() const;
    GLFWwindow* getWindow() const;
  };

  // Global pointer to the instance of the core
  extern Core* CORE;
  extern ECS::World* WORLD;
  extern ObjectFactory* FACTORY;
  extern Graphics* GRAPHICS;
}