#pragma once

#include <string>
#include "SettingsManager.h"

namespace Engine
{
  namespace FileIO
  {
    namespace {
      struct UserSettings
      {
        int window_width = 1280;
        int window_height = 720;
      };

      SettingsManager<UserSettings> user_settings_manager{};
    }

    bool load_user_settings(const std::string& file_path)
    {
      return user_settings_manager.load(file_path);
    }

    bool save_user_settings(const std::string& file_path)
    {
      return user_settings_manager.save(file_path);
    }

    // settings accessor (can be modified by caller)
    UserSettings& get_user_settings()
    {
      return user_settings_manager.settings;
    }
  }
}
