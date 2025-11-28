//-----------------------------------------------------------------------------------
// file:    GameplayState.h
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


class GameplayState : public Engine::Scene
{
  Engine::Audio::Sound bgm;

public:
  void load() {
    Engine::Resources::open_level("levels/LevelOne.json");

    bgm = Engine::Resources::load_sound("sounds/BGM/Level 1 BGM.wav");
    Engine::Audio::play_sound(bgm, 0.3, true); // TODO: get from settings
  };

  void init() {};

  void update() {};

  void draw() {};

  void free() {};

  void unload() {
    Engine::Audio::stop_all_sounds();
  };
};