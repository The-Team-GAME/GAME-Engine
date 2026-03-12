//-----------------------------------------------------------------------------------
// file:    SettingsManager.h
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   This file declares the SettingsManager template class for managing
//          application settings with JSON serialization support. Handles automatic
//          defaults and file I/O operations.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once

#include <string>
#include <optional>
#include "../Math/Vector2D.h"
#include "../Reflection/ReflectionLibrary/Macros.h"

/*
Settings Documentation:

Settings are automatically serialized to and from JSON files using reflection.

- When you add a new member to a settings struct (e.g., `EngineSettings` in `Engine/FileIO/EngineSettings.h` or `UserSettings` in `Engine/FileIO/UserSettings.h`), the new field is **automatically included in the JSON** on the next engine run. **No manual JSON editing is required!!**
- To add a new setting:
  1. Add the member to the struct.
  2. Include it in the `REFLECT_FIELDS` macro at the bottom of the file.

The `SettingsManager` handles loading, merging defaults for missing fields, and saving automatically.

Usage Example:
  // Define your settings struct
  struct MySettings { int value = 42; };
  REFLECT_FIELDS(MySettings, value)

  // Use the settings manager
  SettingsManager<MySettings>::load("config.json");
  auto& settings = SettingsManager<MySettings>::get();
  settings.value = 100;
  SettingsManager<MySettings>::save("config.json");
*/

namespace Engine {
  namespace FileIO {
    template<typename T>
    class SettingsManager {
    private:
      static T settings_;
      static std::string file_path_;

      /*=================================================================================
      Function Brief:
      Internal implementation for loading settings from a JSON file.
      ==================================================================================*/
      static bool load_impl(const std::string& file_path);

      /*=================================================================================
      Function Brief:
      Internal implementation for saving settings to a JSON file.
      ==================================================================================*/
      static bool save_impl(const std::string& file_path);

    public:
      /*=================================================================================
      Function Brief:
      Loads settings from a JSON file. If the file doesn't exist, creates it with
      default settings. If the file exists but is missing fields, merges them from
      defaults and saves the updated file.

      Parameters:
      file_path - Path to the settings JSON file.

      Returns:
      True if settings were loaded or created successfully, false on error.

      Where to call it:
      During application initialization to load user preferences or configuration.
      ==================================================================================*/
      static bool load(const std::string& file_path);

      /*=================================================================================
      Function Brief:
      Saves current settings to a JSON file. Creates the directory path if it doesn't
      exist. If no file path is provided, uses the last loaded file path.

      Parameters:
      file_path - (Optional) Path to the settings JSON file. If empty, uses last loaded path.

      Returns:
      True if settings were saved successfully, false on error.

      Where to call it:
      When settings are modified and need to be persisted, or when creating default
      settings files.
      ==================================================================================*/
      static bool save(const std::string& file_path = "");

      /*=================================================================================
      Function Brief:
      Returns a reference to the current settings instance for reading.

      Returns:
      Const reference to the settings object.

      Where to call it:
      When you need to read settings values.
      ==================================================================================*/
      static const T& get();

      /*=================================================================================
      Function Brief:
      Returns a mutable reference to the current settings instance for modification.

      Returns:
      Mutable reference to the settings object.

      Where to call it:
      When you need to modify settings values at runtime.
      ==================================================================================*/
      static T& get_mutable();

      /*=================================================================================
      Function Brief:
      Resets settings to their default values.

      Where to call it:
      When you want to restore default settings.
      ==================================================================================*/
      static void reset();
    };

    // Static member initialization
    template<typename T>
    T SettingsManager<T>::settings_{};

    template<typename T>
    std::string SettingsManager<T>::file_path_{};
  }
}


#include "SettingsManager.inl"