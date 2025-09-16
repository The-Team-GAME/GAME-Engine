#pragma once

#include "Graphics.h"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include "ECS/ECS.h"
#include "ObjectFactory.h"
#include "Time.h"
#include "ImGuiManager.h"
#include "WindowManager.h"

namespace Engine
{
  class Core
  {
  private:
    bool isRunning;

  public:
    Core();
    ~Core();

    void run();
  };

  extern ECS::World WORLD;
  extern ObjectFactory FACTORY;
  extern Graphics GRAPHICS;
  extern ImGuiManager IMGUI;
  extern WindowManager WINDOW;
  extern Time TIME;
}