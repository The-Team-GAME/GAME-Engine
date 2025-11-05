#pragma once

#include <string>
#include <vector>
#include "../Components/Components.h"
#include "../Reflection/ReflectionLibrary.h"
#include "SettingsManager.h"

namespace Engine
{
  namespace FileIO
  {
    // Values that need to be loaded before initializing engine subsystems (e.g. window size, title)
    struct EngineSettings
    {
      int window_width = 1280;
      int window_height = 720;
      std::string window_title = "Monkey Stole My GPA";
      bool imgui_viewport_enabled = true;
    };

    // Values that need to be loaded after intializing engine subsystems, (e.g. Textures need to wait for Graphics init)
    struct ProjectSettings
    {
      std::vector<Components::AtlasEntry> loaded_tilemaps{};
    };

    // Values that are specific to the local user/machine (e.g. ImGui theme)
    struct LocalSettings
    {
      std::string imgui_theme = "ayu-dark";
    };
  }
}

REFLECT_FIELDS(Engine::FileIO::EngineSettings, window_title, window_width, window_height)
REFLECT_FIELDS(Engine::FileIO::ProjectSettings, loaded_tilemaps)
REFLECT_FIELDS(Engine::FileIO::LocalSettings, imgui_theme)
