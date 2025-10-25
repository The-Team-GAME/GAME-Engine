#pragma once

#include <string>
#include "SettingsManager.h"
#include "../Reflection/ReflectionLibrary.h"

namespace Engine
{
  namespace FileIO
  {
    struct EngineSettings
    {
      std::string window_title = "Monkey Stole My GPA";
    };

    // Private static instance of SettingsManager for EngineSettings
    namespace {
      static SettingsManager<EngineSettings> engine_settings_manager{};
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
    const EngineSettings& get_engine_settings()
    {
      return engine_settings_manager.settings;
    }
  }
}

REFLECT_FIELDS(Engine::FileIO::EngineSettings, window_title)