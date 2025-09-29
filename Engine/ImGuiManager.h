#pragma once

#include <functional>
#include <vector>

namespace Engine
{
  class ImGuiManager
  {
  private:
    bool guiEnabled = true;

  public:
    ImGuiManager();
    ~ImGuiManager();

    void begin();
    void end();

    void update();
  };
}