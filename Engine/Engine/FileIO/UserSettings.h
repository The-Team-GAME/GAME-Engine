#pragma once

#include <string>
#include "SettingsManager.h"

namespace Engine
{
  namespace FileIO
  {
    struct UserSettings
    {
      int window_width = 1280;
      int window_height = 720;
    };

    // Convenience functions for UserSettings
    inline bool load_user_settings(const std::string& file_path)
    {
      return SettingsManager<UserSettings>::load(file_path);
    }

    inline bool save_user_settings(const std::string& file_path = "")
    {
      return SettingsManager<UserSettings>::save(file_path);
    }

    // settings accessor (can be modified by caller)
    inline UserSettings& get_user_settings()
    {
      return SettingsManager<UserSettings>::get_mutable();
    }
  }
}

REFLECT_FIELDS(Engine::FileIO::UserSettings, window_width, window_height)