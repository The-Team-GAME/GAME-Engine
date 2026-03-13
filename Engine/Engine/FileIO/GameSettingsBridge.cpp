//-----------------------------------------------------------------------------------
// file:    GameSettingsBridge.cpp
// project: Monkey Stole My GPA
//
// brief:   Implements the ENGINE_API bridge for GameSettings by forwarding to the
//          single SettingsManager<GameSettings> instance that lives in the Engine
//          EXE. This ensures GameLogic DLL operations hit the same settings_ and
//          file_path_ as the Engine.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------

#include "GameSettingsBridge.h"
#include "Settings.h"       // GameSettings + SettingsManager
#include "SettingsManager.h"

namespace Engine {
  namespace FileIO {
    namespace GameSettingsBridge {

      const GameSettings& get()
      {
        return SettingsManager<GameSettings>::get();
      }

      GameSettings& get_mutable()
      {
        return SettingsManager<GameSettings>::get_mutable();
      }

      bool save()
      {
        return SettingsManager<GameSettings>::save();
      }

      bool load(const std::string& file_path)
      {
        return SettingsManager<GameSettings>::load(file_path);
      }

    } // namespace GameSettingsBridge
  } // namespace FileIO
} // namespace Engine
