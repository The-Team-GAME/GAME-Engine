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
*/

namespace Engine {
  namespace FileIO {
    template<typename T>
    class SettingsManager {
    public:
      T settings{};

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
      bool load(const std::string& file_path);

      /*=================================================================================
      Function Brief:
      Saves current settings to a JSON file. Creates the directory path if it doesn't
      exist.

      Parameters:
      file_path - Path to the settings JSON file.

      Returns:
      True if settings were saved successfully, false on error.

      Where to call it:
      When settings are modified and need to be persisted, or when creating default
      settings files.
      ==================================================================================*/
      bool save(const std::string& file_path);
    };
  }
}


#include "SettingsManager.inl"