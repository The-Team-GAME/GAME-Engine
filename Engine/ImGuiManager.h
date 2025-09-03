#pragma once

#include <functional>
#include <vector>

namespace Engine
{
  class ImGuiManager
  {
  public:
    using Panel = std::function<void()>;

  private:
    static std::vector<Panel> panels;

  public:
    static void init();
    static void update();
    static void addPanel(Panel panel);
  };
}