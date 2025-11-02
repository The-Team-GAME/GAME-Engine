#pragma once

#include <string>
#include <vector>
#include "../Components/Components.h"

namespace Engine
{
  namespace FileIO
  {
    struct UserSettings
    {
      int window_width = 1280;
      int window_height = 720;
    };

    struct EngineSettings
    {
      std::string window_title = "Monkey Stole My GPA";
    };

    struct ProjectSettings
    {
      std::vector<Components::AtlasEntry> loaded_tilemaps{};
    };

    struct LocalSettings
    {
      std::string imgui_theme = "Moonlight";
    };
  }
}
