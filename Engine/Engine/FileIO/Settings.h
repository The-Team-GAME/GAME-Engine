#pragma once

#include <string>
#include <vector>
#include "../Components/Components.h"
#include "../Reflection/ReflectionLibrary.h"

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

REFLECT_FIELDS(Engine::FileIO::UserSettings, window_width, window_height)
REFLECT_FIELDS(Engine::FileIO::EngineSettings, window_title)
REFLECT_FIELDS(Engine::FileIO::ProjectSettings, loaded_tilemaps)
REFLECT_FIELDS(Engine::FileIO::LocalSettings, imgui_theme)
