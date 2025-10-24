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

Settings are automatically saved to JSON files using reflection.

When you add a new member to a settings struct (like `EngineSettings` or `UserSettings`),
the new field will automatically be added to the json the next time you run the Engine!!
So no need to add it into the json manually.

To add a new setting:
1. Add the member to the struct
2. Add it to the `REFLECT_FIELDS` macro at the bottom of the file

The `SettingsManager` handles loading, merging defaults, and saving automatically.
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