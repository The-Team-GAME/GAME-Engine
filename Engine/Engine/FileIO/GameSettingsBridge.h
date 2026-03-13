//-----------------------------------------------------------------------------------
// file:    GameSettingsBridge.h
// project: Monkey Stole My GPA
//
// brief:   ENGINE_API bridge functions for GameSettings, ensuring that both the
//          Engine EXE and GameLogic DLL operate on the same SettingsManager
//          instance. Without this bridge, each module gets its own static copy
//          of the SettingsManager<GameSettings> template (separate settings_
//          and file_path_), so saves/reads from the DLL are silently broken.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once

#include <string>
#include "../EngineExport.h"

namespace Engine {
  namespace FileIO {

    struct GameSettings; // forward declaration

    namespace GameSettingsBridge {

      // Returns a const reference to the Engine's GameSettings instance.
      ENGINE_API const GameSettings& get();

      // Returns a mutable reference to the Engine's GameSettings instance.
      ENGINE_API GameSettings& get_mutable();

      // Saves the Engine's GameSettings to disk (uses the path from load()).
      ENGINE_API bool save();

      // Loads GameSettings from the given file path into the Engine's instance.
      ENGINE_API bool load(const std::string& file_path);

    } // namespace GameSettingsBridge
  } // namespace FileIO
} // namespace Engine
