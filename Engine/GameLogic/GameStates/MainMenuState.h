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
  void load() {
    Engine::FunctionRegistry<void()>::register_function("load_level_test", []() {
      Engine::SceneManager::load_scene("GameplayState");
      });

    Engine::FunctionRegistry<void()>::register_function("exit_game", []() {
      Engine::Core::stop();
      });
    Engine::Resources::open_level("levels/MainMenu.json");
  };

  void init() {};

  void update() {};

  void draw() {
  };

  void free() {};

  void unload() {
  };
};