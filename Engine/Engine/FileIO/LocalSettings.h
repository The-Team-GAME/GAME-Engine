#pragma once

#include <string>
#include "SettingsManager.h"

namespace Engine
{
  namespace FileIO
  {
    struct LocalSettings
    {
      std::string imgui_theme = "Moonlight";
    };

    // Private static instance of SettingsManager for LocalSettings
    namespace {
      static SettingsManager<LocalSettings> local_settings_manager{};
    }

    bool load_local_settings(const std::string& file_path)
    {
      return local_settings_manager.load(file_path);
    }

    bool save_local_settings(const std::string& file_path)
    {
      return local_settings_manager.save(file_path);
    }

    // settings accessor (can be modified by caller)
    LocalSettings& get_local_settings()
    {
      return local_settings_manager.settings;
    }
  }
}

REFLECT_FIELDS(Engine::FileIO::LocalSettings, imgui_theme)