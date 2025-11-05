//-----------------------------------------------------------------------------------
// file:    Settings.h
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   This file defines the settings structures used by the engine for
//          configuration management. Settings are automatically serialized to/from
//          JSON files using reflection. Default values are used only when creating
//          new JSON files, not as hard-coded runtime values.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once

#include <string>
#include <vector>
#include "../Components/Components.h"
#include "../Reflection/ReflectionLibrary.h"
#include "SettingsManager.h"

/*
Settings Documentation:

This file defines the settings structures used by the engine. Settings are automatically
serialized to and from JSON files using the reflection system and SettingsManager.

DEFAULT VALUES:
- The default values specified in these structs (e.g., window_width = 1280) are NOT hard-coded
  into the application at runtime.
- These defaults are ONLY used when the JSON settings file is created for the first time.
- Once the JSON file exists, the engine loads values from the file, not from these defaults.
- The values from the JSON files take precedence over these defaults.
- If a field is missing from an existing JSON file, the default value will be added automatically.

ADDING NEW SETTINGS:
1. Add the member variable to the appropriate settings struct with its default value.
2. Include it in the REFLECT_FIELDS macro at the bottom of this file.
3. The new field will automatically appear in the JSON file on the next engine run.

SETTINGS TYPES:
- EngineSettings: Values needed before engine subsystems initialize (e.g., window configuration).
- ProjectSettings: Values needed after engine initialization (e.g., assets requiring Graphics system).
- LocalSettings: User/machine-specific values (e.g., UI theme preferences).

See SettingsManager.h for more details on loading and saving settings.
*/

namespace Engine
{
  namespace FileIO
  {
    // Values that need to be loaded before initializing engine subsystems (e.g. window size, title)
    struct EngineSettings
    {
      int window_width = 1280;
      int window_height = 720;
      std::string window_title = "Monkey Stole My GPA";
      bool imgui_viewport_enabled = true;
    };

    // Values that need to be loaded after intializing engine subsystems, (e.g. Textures need to wait for Graphics init)
    struct ProjectSettings
    {
      std::vector<Components::AtlasEntry> loaded_tilemaps{};
    };

    // Values that are specific to the local user/machine (e.g. ImGui theme)
    struct LocalSettings
    {
      std::string imgui_theme = "ayu-dark";
    };
  }
}

REFLECT_FIELDS(Engine::FileIO::EngineSettings, window_title, window_width, window_height)
REFLECT_FIELDS(Engine::FileIO::ProjectSettings, loaded_tilemaps)
REFLECT_FIELDS(Engine::FileIO::LocalSettings, imgui_theme)
