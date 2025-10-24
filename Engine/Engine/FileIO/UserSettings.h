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

    SettingsManager<UserSettings> manager{};

    // settings accessor (can be modified by caller)
    UserSettings& get_user_settings()
    {
      return manager.settings;
    }
  }
}
