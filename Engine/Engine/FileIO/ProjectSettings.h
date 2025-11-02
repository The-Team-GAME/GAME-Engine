#pragma once

#include <string>
#include "SettingsManager.h"
#include "../Reflection/ReflectionLibrary.h"

#include "../Components/TilemapComponents.h"

namespace Engine
{
  namespace FileIO
  {
    struct ProjectSettings
    {
      std::vector<Components::AtlasEntry> loaded_tilemaps{};
    };

    // Private static instance of SettingsManager for ProjectSettings
    namespace {
      static SettingsManager<ProjectSettings> project_settings_manager{};
    }

    bool load_project_settings(const std::string& file_path)
    {
      return project_settings_manager.load(file_path);
    }

    bool save_project_settings(const std::string& file_path)
    {
      return project_settings_manager.save(file_path);
    }

    // settings accessor (can be modified by caller)
    ProjectSettings& get_project_settings()
    {
      return project_settings_manager.settings;
    }
  }
}

REFLECT_FIELDS(Engine::FileIO::ProjectSettings, loaded_tilemaps)