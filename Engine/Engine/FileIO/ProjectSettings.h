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
      static SettingsManager<ProjectSettings> engine_settings_manager{};
    }

    bool load_engine_settings(const std::string& file_path)
    {
      return engine_settings_manager.load(file_path);
    }

    bool save_engine_settings(const std::string& file_path)
    {
      return engine_settings_manager.save(file_path);
    }

    // settings accessor (cannot be modified by caller, we don't want engine settings changed at runtime)
    const ProjectSettings& get_engine_settings()
    {
      return engine_settings_manager.settings;
    }
  }
}

REFLECT_FIELDS(Engine::FileIO::ProjectSettings, loaded_tilemaps)