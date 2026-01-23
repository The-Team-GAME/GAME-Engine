//-----------------------------------------------------------------------------------
// file:    MainMenuCallbacks.h
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   Level callbacks for the main menu level. Registers on_init, on_update,
//          and on_exit functions that replace the old MainMenuState scene.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once

#include "../../Engine/MonkeyEngine.h"

namespace LevelCallbacks {
  namespace MainMenu {

    namespace {
      Engine::Audio::Sound bgm;
    }

    inline void on_init() {
      // Register button callbacks for the main menu UI
      Engine::FunctionRegistry<void()>::register_function("start_game", []() {
        Engine::LevelManager::open_level("levels/LevelOne.json");
        });
      Engine::FunctionRegistry<void()>::register_function("options_menu", []() {
        // TODO: Implement options as overlay or separate level
        });
      Engine::FunctionRegistry<void()>::register_function("exit_game", []() {
        Engine::Core::stop();
        });

      bgm = Engine::Resources::load_sound("sounds/BGM/Main Menu/Ricoche_LO-fi_Cut-Scene_Loop_120bpm.wav");
      Engine::Audio::play_sound(bgm, 0.3f, true); // TODO: get from settings
    }

    inline void on_update() {
      // Main menu update logic (if any)
    }

    inline void on_exit() {
      Engine::Audio::stop_all_sounds();
    }

    // Register all main menu level callbacks
    inline void register_callbacks() {
      Engine::FunctionRegistry<void()>::register_function("MainMenu_Init", on_init);
      Engine::FunctionRegistry<void()>::register_function("MainMenu_Update", on_update);
      Engine::FunctionRegistry<void()>::register_function("MainMenu_Exit", on_exit);
    }

  } // namespace MainMenu
} // namespace LevelCallbacks
