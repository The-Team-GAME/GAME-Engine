//-----------------------------------------------------------------------------------
// file:    MainMenuState.h
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once

/* EVERY CODE HERE IS TEMPORARY */

#include "../../Engine/MonkeyEngine.h"

 
class MainMenuState : public Engine::Scene
{
void main_menu_quit() {
  Engine::Core::stop();
}

void main_menu_start() {
  Engine::SceneManager::load_scene("GameplayState");
}

public:
  Engine::Audio::Sound bgm;
  void load() { 
    Engine::FunctionRegistry<void()>::register_function("start_game", []() {
      Engine::SceneManager::load_scene("GameplayState");
      });
    Engine::FunctionRegistry<void()>::register_function("options_menu", []() {
      // Engine::SceneManager::push_scene("OptionsState");
      });
    Engine::FunctionRegistry<void()>::register_function("exit_game", []() {
      Engine::Core::stop();
      });
    Engine::Resources::open_level("levels/MainMenu.json");

    bgm = Engine::Resources::load_sound("sounds/BGM/Main Menu/Ricoche_LO-fi_Cut-Scene_Loop_120bpm.wav");
    Engine::Audio::play_sound(bgm, 0.3f, true); // TODO: get from settings
  };

  void init() {};

  void update() {};

  void draw() {
  };

  void free() {};

  void unload() {
    Engine::Audio::stop_all_sounds();
  };
};