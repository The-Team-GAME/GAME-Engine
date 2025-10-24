#pragma once

#include <string>
#include "SettingsManager.h"

namespace Engine
{
  namespace FileIO
  {
    struct EngineSettings
    {
      int example = 42;
    };

    SettingsManager<EngineSettings> manager{};

    // settings accessor (cannot be modified by caller, we don't want engine settings changed at runtime)
    const EngineSettings& get_engine_settings()
    {
      return manager.settings;
    }
  }
}
