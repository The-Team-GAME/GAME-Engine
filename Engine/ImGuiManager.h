#pragma once

#include <functional>
#include <vector>

namespace Engine
{
  class ImGuiManager
  {
  public:
    ImGuiManager();
    ~ImGuiManager();

    void begin();
    void end();
  };
}