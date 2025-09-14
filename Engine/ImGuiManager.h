#pragma once

#include <functional>
#include <vector>

namespace Engine
{
  class ImGuiManager
  {
  public:
    static void init();
    static void begin();
    static void end();
    static void exit();
  };
}