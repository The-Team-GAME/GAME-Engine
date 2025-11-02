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

    // Convenience functions for LocalSettings
    inline bool load_local_settings(const std::string& file_path)
    {
      return SettingsManager<LocalSettings>::load(file_path);
    }

    inline bool save_local_settings(const std::string& file_path = "")
    {
      return SettingsManager<LocalSettings>::save(file_path);
    }

    // settings accessor (can be modified by caller)
    inline LocalSettings& get_local_settings()
    {
      return SettingsManager<LocalSettings>::get_mutable();
    }
  }
}

REFLECT_FIELDS(Engine::FileIO::LocalSettings, imgui_theme)