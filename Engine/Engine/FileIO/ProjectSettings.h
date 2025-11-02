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

    // Convenience functions for ProjectSettings
    inline bool load_project_settings(const std::string& file_path)
    {
      return SettingsManager<ProjectSettings>::load(file_path);
    }

    inline bool save_project_settings(const std::string& file_path = "")
    {
      return SettingsManager<ProjectSettings>::save(file_path);
    }

    // settings accessor (can be modified by caller)
    inline ProjectSettings& get_project_settings()
    {
      return SettingsManager<ProjectSettings>::get_mutable();
    }
  }
}

REFLECT_FIELDS(Engine::FileIO::ProjectSettings, loaded_tilemaps)