#pragma once

#include "Precompiled.h"
#include "Graphics.h"
#include "ECS/ECS.h"
#include "ObjectFactory.h"
#include "Time.h"
#include "ImGuiManager.h"
#include "WindowManager.h"
#include "AudioManager.h"
#include "ResourceManager.h"

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
  extern AudioManager AUDIO;
  extern ResourceManager RESOURCES;
}